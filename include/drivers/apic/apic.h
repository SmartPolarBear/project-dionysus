#if !defined(__INCLUDE_DRIVERS_APIC_H)
#define __INCLUDE_DRIVERS_APIC_H

#include "sys/types.h"

namespace apic
{
extern volatile uint32_t *lapic;
}

#endif // __INCLUDE_DRIVERS_APIC_H
