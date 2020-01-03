#if !defined(__INCLUDE_DRIVERS_APIC_TIMER_TIMER_H)
#define __INCLUDE_DRIVERS_APIC_TIMER_TIMER_H

namespace timer
{
void init_apic_timer(void);
void handle_tick(void);
} // namespace timer

#endif // __INCLUDE_DRIVERS_APIC_TIMER_TIMER_H
