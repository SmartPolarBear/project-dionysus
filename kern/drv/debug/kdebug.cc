#include "arch/amd64/cpu/x86.h"

#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "debug/backtrace.hpp"

#include "drivers/acpi/cpu.h"

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
	console::console_set_lock(false);
	// change cga color to draw attention
	console::console_set_color(clr.first, clr.second);

	if (topleft)
	{
		console::console_set_pos(0);
	}

	write_format("[CPU%d] ", cpu->id);

	valist_write_format(fmt, ap);

//	constexpr size_t PCS_BUFLEN = 16;
//	uintptr_t pcs[PCS_BUFLEN] = { 0 };
//	kdebug::kdebug_get_caller_pcs(PCS_BUFLEN, pcs);
//
//	write_format("\n");
//	for (auto pc : pcs)
//	{
//		write_format("%p ", pc);
//	}
	kdebug_print_backtrace();

	// restore cga color
	console::console_set_color(cs_default.first, cs_default.second);

	write_format("\n");
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

