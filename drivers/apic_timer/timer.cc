#include "drivers/apic_timer/timer.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"

using local_apic::TDCR;
using local_apic::TIC_DEFUALT_VALUE;
using local_apic::TICR;
using local_apic::TIMER;
using local_apic::TIMER_FLAG_PERIODIC;
using local_apic::TIMER_FLAG_X1;

using local_apic::write_lapic;

void timer::init_apic_timer(void)
{
    // initialize timer
    write_lapic(TDCR, TIMER_FLAG_X1);
    write_lapic(TIMER, TIMER_FLAG_PERIODIC | (TRAP_IRQ0 + IRQ_TIMER));
    write_lapic(TICR, TIC_DEFUALT_VALUE);
}
