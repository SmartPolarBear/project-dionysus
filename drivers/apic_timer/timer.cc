#include "drivers/apic_timer/timer.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
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

void timer::init_apic_timer(void)
{
    // initialize timer
    write_lapic(TDCR, TIMER_FLAG_X1);
    write_lapic(TIMER, TIMER_FLAG_PERIODIC | (TRAP_IRQ0 + IRQ_TIMER));
    write_lapic(TICR, TIC_DEFUALT_VALUE);

    spinlock_initlock(&tickslock, "timer_ticks");
}

void timer::handle_tick(void)
{
    if (cpu->id == 0)
    {
        spinlock_acquire(&tickslock);
        ticks++;
        //TODO: wake up processes
        spinlock_release(&tickslock);
    }
    local_apic::write_eoi();
}
