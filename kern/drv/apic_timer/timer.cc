#include "arch/amd64/regs.h"

#include "sys/error.h"
#include "sys/types.h"
#include "sys/scheduler.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic_timer/timer.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

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

uint64_t ticks = 0;
spinlock tickslock;

// defined below
error_code trap_handle_tick(trap_frame info);

PANIC void timer::init_apic_timer(void)
{
    // register the handle
    trap::trap_handle_regsiter(trap::irq_to_trap_number(IRQ_TIMER), trap::trap_handle{.handle = trap_handle_tick});
    // initialize apic values
    write_lapic(TDCR, TIMER_FLAG_X1);
    write_lapic(TIMER, TIMER_FLAG_PERIODIC | (trap::irq_to_trap_number(IRQ_TIMER)));
    write_lapic(TICR, TIC_DEFUALT_VALUE);

    spinlock_initlock(&tickslock, "timer_ticks");
}

error_code trap_handle_tick([[maybe_unused]] trap_frame info)
{
    if (cpu->id == 0)
    {
        spinlock_acquire(&tickslock);
        ticks++;

        spinlock_release(&tickslock);

        // scheduler::scheduler_yield();
    }

    return ERROR_SUCCESS;
}
