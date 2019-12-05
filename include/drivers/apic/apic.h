#if !defined(__INCLUDE_DRIVERS_APIC_H)
#define __INCLUDE_DRIVERS_APIC_H

#include "sys/types.h"

namespace local_apic
{
extern volatile uint32_t *lapic;
void init_lapic(void);
size_t get_cpunum(void);
} // namespace local_apic

namespace io_apic
{
    
} // namespace ioapic


#endif // __INCLUDE_DRIVERS_APIC_H
