#include "arch/amd64/cpu/x86.h"

#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "system/memlayout.h"
#include "system/types.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include <utility>

using std::make_pair;

using color_scheme = std::pair<console::console_colors, console::console_colors>;

constexpr color_scheme cs_default{ console::CONSOLE_COLOR_BLACK, console::CONSOLE_COLOR_LIGHT_GREY };
constexpr color_scheme cs_panic{ console::CONSOLE_COLOR_BLUE, console::CONSOLE_COLOR_LIGHT_BROWN };
constexpr color_scheme cs_warning{ console::CONSOLE_COLOR_BLUE, console::CONSOLE_COLOR_WHITE };

bool kdebug::panicked = false;

using namespace kdebug;

static inline void kdebug_print_impl_v(const char* fmt, bool topleft, color_scheme clr, va_list ap)
{
	// change cga color to draw attention
	console::console_set_color(clr.first, clr.second);

	if (topleft)
	{
		console::console_set_pos(0);
	}

	valist_write_format(fmt, ap);

	constexpr size_t PCS_BUFLEN = 16;
	uintptr_t pcs[PCS_BUFLEN] = { 0 };
	kdebug::kdebug_get_caller_pcs(PCS_BUFLEN, pcs);

	write_format("\n");
	for (auto pc : pcs)
	{
		write_format("%p ", pc);
	}

	// restore cga color
	console::console_set_color(cs_default.first, cs_default.second);

	write_format("\n");
}

// read information from %rbp and retrive pcs
void kdebug::kdebug_get_caller_pcs(size_t buflen, uintptr_t* pcs)
{
	uintptr_t* ebp = nullptr;
	asm volatile("mov %%rbp, %0"
	: "=r"(ebp));

	size_t i = 0;
	for (; i < buflen; i++)
	{
		if (ebp == nullptr || ebp < (uintptr_t*)KERNEL_VIRTUALBASE || ebp == (uintptr_t*)VIRTUALADDR_LIMIT)
		{
			break;
		}
		pcs[i] = ebp[1];           // saved %eip
		ebp = (uintptr_t*)ebp[0]; // saved %ebp
	}

	for (; i < buflen; i++)
	{
		pcs[i] = 0;
	}
}

[[noreturn]] void kdebug::kdebug_panic(const char* fmt, uint32_t topleft, ...)
{
	// disable interrupts
	cli();

	va_list ap;
	va_start(ap, topleft);

	kdebug_print_impl_v(fmt, topleft, cs_panic, ap);

	va_end(ap);

	// set global panic state for other cpu
	panicked = true;

	// infinite loop
	for (;;);
}

void kdebug::kdebug_warning(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	kdebug_print_impl_v(fmt, false, cs_warning, ap);

	va_end(ap);
}

void kdebug::kdebug_log(const char* fmt, ...)
{
#ifdef DEBUG
	va_list ap;

	va_start(ap, fmt);

	valist_write_format(fmt, ap);

	va_end(ap);
#endif
}

void kdebug::kdebug_dump_lock_panic(lock::spinlock* lock)
{
	// disable the lock of console
	console::console_set_lock(false);
	console::console_set_pos(0);

	console::console_set_color(cs_panic.first, cs_panic.second);

	write_format("lock %s has been held.\nCall stack:\n", lock->name);

	for (auto cs : lock->pcs)
	{
		write_format("%p ", cs);
	}

	write_format("\n");

	KDEBUG_FOLLOWPANIC("acquire");
}