#include "arch/amd64/cpu/x86.h"
#include "arch/amd64/lock/arch_spinlock.hpp"

#include "drivers/console/console.h"
#include "drivers/apic/apic.h"

#include "debug/kdebug.h"
#include "debug/backtrace.hpp"

#include "drivers/acpi/cpu.h"

#include "system/memlayout.h"
#include "system/types.h"

#include "builtin_text_io.hpp"

using namespace kdebug;
using namespace apic;

using namespace local_apic;
using namespace io_apic;

constexpr std::pair<console::console_colors, console::console_colors>
	panic_color{ console::CONSOLE_COLOR_BLUE, console::CONSOLE_COLOR_LIGHT_BROWN };

static inline void panic_print(const char* fmt, bool topleft, va_list ap)
{
	console::console_set_lock(false);
	// change cga color to draw attention
	console::console_set_color(panic_color.first, panic_color.second);

	if (topleft)
	{
		console::console_set_pos(0);
	}

	if (cpu.is_valid())
	{
		write_format("[CPU%d] ", cpu->id);
	}
	else
	{
		write_format("[CPU0 (cpu.is_valid() == false)]");
	}

	valist_write_format(fmt, ap);

	kdebug_print_backtrace();

	write_format("\n");
}

lock::arch_spinlock panic_lock{};

[[noreturn]] void kdebug::kdebug_panic(const char* fmt, uint32_t topleft, ...)
{
	// disable interrupts
	cli();

	// only one attempt can succeeded panicking, otherwise directly go to halt status
	if (lock::arch_spinlock_try_lock(&panic_lock))
	{
		// infinite loop to halt the cpu
		for (;;)
			hlt();
	}

	console::console_panic_lock();


	if (valid_cpus.size() > 1)
	{
		apic::local_apic::apic_broadcast_ipi(DLM_FIXED, trap::IRQ_TO_TRAPNUM(trap::IRQ_HALT_CPU_HANDLE));
	}

	va_list ap;
	va_start(ap, topleft);
	panic_print(fmt, topleft, ap);
	va_end(ap);

	// infinite loop to halt the cpu
	for (;;)
		hlt();
}