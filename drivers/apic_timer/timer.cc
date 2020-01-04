#include "arch/amd64/regs.h"

#include "sys/types.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic_timer/timer.h"
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
hresult handle_tick(trap_info info);

void timer::init_apic_timer(void)
{
    // register the handle
    trap::trap_handle_regsiter(TRAP_IRQ0 + IRQ_TIMER, trap::trap_handle{
                                                          .handle = handle_tick});

    // initialize apic values
    write_lapic(TDCR, TIMER_FLAG_X1);
    write_lapic(TIMER, TIMER_FLAG_PERIODIC | (TRAP_IRQ0 + IRQ_TIMER));
    write_lapic(TICR, TIC_DEFUALT_VALUE);

    spinlock_initlock(&tickslock, "timer_ticks");
}

hresult handle_tick([[maybe_unused]] trap_info info)
{
    if (cpu->id == 0)
    {
        spinlock_acquire(&tickslock);
        ticks++;
        //TODO: wake up processes
        spinlock_release(&tickslock);
    }
    local_apic::write_eoi();

    return HRES_SUCCESS;
}
