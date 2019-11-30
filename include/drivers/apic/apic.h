#if !defined(__INCLUDE_DRIVERS_APIC_H)
#define __INCLUDE_DRIVERS_APIC_H

#include "sys/types.h"

namespace local_apic
{
extern volatile uint32_t *lapic;
void lapic_init(void);
} // namespace lapic

namespace io_apic
{
    
} // namespace ioapic


#endif // __INCLUDE_DRIVERS_APIC_H
