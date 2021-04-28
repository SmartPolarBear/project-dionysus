#pragma once

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic_resgiters.hpp"

#include "system/types.h"

namespace apic
{

namespace local_apic
{

constexpr size_t TIC_DEFUALT_VALUE = 10000000;

extern volatile uint8_t* lapic_base;

PANIC void init_lapic();
size_t get_cpunum();
void write_eoi();

void write_lapic(offset_t addr_off, uint32_t value);
void write_lapic(offset_t addr_off, uint64_t value);

template<_internals::APICRegister T>
static inline T read_lapic(offset_t addr_off)
{
	return *((T*)(volatile void*)(&local_apic::lapic_base[addr_off]));
}

template<_internals::APICLongRegister T>
static inline T read_lapic(offset_t addr_off)
{
	return *((T*)(volatile void*)(&local_apic::lapic_base[addr_off]));
}

template<_internals::APICRegister T>
static inline void write_lapic(offset_t addr_off, T val)
{
	write_lapic(addr_off, *((uint32_t*)(void*)(&val)));
}

template<_internals::APICLongRegister T>
static inline void write_lapic(offset_t addr_off, T val)
{
	write_lapic(addr_off, *((uint64_t*)(void*)(&val)));
}

/// \brief the dst_apic_id for a broadcast ipi, which means all APICs
static inline constexpr uint32_t BROADCAST_DST_APIC_ID = 0;

void apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode,
	uint32_t vec, irq_destinations irq_dest);

void apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode, uint32_t vec);

void apic_broadcast_ipi(delievery_modes mode, uint32_t vec);

void start_ap(size_t apicid, uintptr_t addr);

} // namespace local_apic


}