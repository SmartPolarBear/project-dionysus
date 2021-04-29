#pragma once

#include "drivers/acpi/cpu.h"
#include "drivers/apic/ioapic_registers.hpp"

#include "system/types.h"

namespace apic::io_apic
{

struct redtbl_reg
{
	offset_t low, high;
};

constexpr redtbl_reg IOAPIC_REDTBL_ENTRY_REG(size_t index)
{
	auto low = IOREDTBL_BASE + index * 2 + 0, high = IOREDTBL_BASE + index * 2 + 1;
	return { low, high };
}

void write_ioapic(uintptr_t apic_base, uint8_t reg, uint32_t val);
uint32_t read_ioapic(uintptr_t apic_base, uint8_t reg);

template<IOAPICRegister T>
void write_ioapic(uintptr_t apic_base, uint8_t reg, T val)
{
	write_ioapic(apic_base, reg, *((uint32_t*)(void*)(&val)));
}

template<IOAPICRegister T>
T read_ioapic(uintptr_t apic_base, uint8_t reg)
{
	auto val = read_ioapic(apic_base, reg);
	return *((T*)(volatile void*)(&val));
}

PANIC void init_ioapic();
void write_redtbl(uint32_t trapnum, uint32_t cpu_acpi_id_rounted);

} // namespace io_apic
