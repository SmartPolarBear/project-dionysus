#if !defined(__INCLUDE_DRIVERS_APIC_TIMER_TIMER_H)
#define __INCLUDE_DRIVERS_APIC_TIMER_TIMER_H
#include "sys/types.h"

namespace timer
{
PANIC void init_apic_timer(void);
} // namespace timer

#endif // __INCLUDE_DRIVERS_APIC_TIMER_TIMER_H
