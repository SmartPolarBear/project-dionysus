#include "arch/amd64/cpu/regs.h"

#include "system/error.hpp"
#include "system/scheduler.h"
#include "system/types.h"

#include "drivers/apic/timer.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"
#include "drivers/acpi/cpu.h"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"
#include "task/process/process.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using local_apic::TDCR;
using local_apic::TIC_DEFUALT_VALUE;
using local_apic::TICR;
using local_apic::TIMER;
using local_apic::TIMER_FLAG_PERIODIC;
using local_apic::TIMER_FLAG_X1;

using local_apic::write_lapic;

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;

using trap::IRQ_TIMER;
using trap::TRAP_IRQ0;

uint64_t ticks[CPU_COUNT_LIMIT] = { 0 };

spinlock_struct tickslocks[CPU_COUNT_LIMIT] = {};

bool enable_irq[CPU_COUNT_LIMIT] = { false };

static inline spinlock_struct* ticks_lock()
{
	return &tickslocks[cpu->id];
}

// defined below
error_code trap_handle_tick(trap::trap_frame info);

PANIC void timer::setup_apic_timer()
{
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

	spinlock_initialize_lock(ticks_lock(), "timer_ticks");
}

error_code trap_handle_tick([[maybe_unused]] trap::trap_frame info)
{
	size_t id = cpu->id;

	if (kdebug::panicked)
	{
		hlt();
		for (;;);
	}

	if (enable_irq[id])
	{
		spinlock_acquire(ticks_lock());
		ticks[id]++;
		spinlock_release(ticks_lock());

		local_apic::write_eoi();

		cpu->scheduler.handle_timer_tick();

	}

	return ERROR_SUCCESS;
}

void timer::set_enable_on_cpu(size_t cpuid, bool enable)
{
	enable_irq[cpuid] = enable;
}
uint64_t timer::get_ticks()
{
	return ticks[cpu->id];
}
