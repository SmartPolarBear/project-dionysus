#include "arch/amd64/regs.h"

#include "system/error.h"
#include "system/scheduler.h"
#include "system/types.h"

#include "drivers/apic/timer.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"
#include "drivers/acpi/cpu.h"

#include "libraries/libkernel/console/builtin_console.hpp"

using local_apic::TDCR;
using local_apic::TIC_DEFUALT_VALUE;
using local_apic::TICR;
using local_apic::TIMER;
using local_apic::TIMER_FLAG_PERIODIC;
using local_apic::TIMER_FLAG_X1;

using local_apic::write_lapic;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

using trap::IRQ_TIMER;
using trap::TRAP_IRQ0;

uint64_t ticks[CPU_COUNT_LIMIT] = { 0 };

spinlock tickslocks[CPU_COUNT_LIMIT] = {};

bool enable_irq[CPU_COUNT_LIMIT] = { false };

static inline spinlock* ticks_lock()
{
	return &tickslocks[cpu()->id];
}

// defined below
error_code trap_handle_tick(trap::trap_frame info);

PANIC void timer::setup_apic_timer()
{
	return;
	// initialize apic values
	write_lapic(TDCR, TIMER_FLAG_X1);
	write_lapic(TIMER, TIMER_FLAG_PERIODIC | (trap::irq_to_trap_number(IRQ_TIMER)));
	write_lapic(TICR, TIC_DEFUALT_VALUE);
}

PANIC void timer::init_apic_timer()
{
	// register the handle
	trap::trap_handle_register(trap::irq_to_trap_number(IRQ_TIMER),
		trap::trap_handle
			{
				.handle = trap_handle_tick,
				.enable = true
			});

	spinlock_initlock(ticks_lock(), "timer_ticks");
}

error_code trap_handle_tick([[maybe_unused]] trap::trap_frame info)
{
	size_t id = cpu()->id;

	if (enable_irq[id])
	{
		spinlock_acquire(ticks_lock());
		ticks[id]++;

//		write_format("ticks=%lld", ticks[id]);

		spinlock_release(ticks_lock());

//		scheduler::scheduler_yield();
	}

	return ERROR_SUCCESS;
}

void timer::set_enable_on_cpu(size_t cpuid, bool enable)
{
	enable_irq[cpuid] = enable;
}
