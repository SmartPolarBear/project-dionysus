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
void init_ioapic(void);
void enable_trap(uint32_t trapnum, uint32_t cpu_rounted);
} // namespace io_apic

namespace pic8259A
{

void initialize_pic(void);

} // namespace pic8259A

#endif // __INCLUDE_DRIVERS_APIC_H
