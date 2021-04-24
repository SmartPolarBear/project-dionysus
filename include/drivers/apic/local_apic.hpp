#pragma once

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic_resgiters.hpp"

#include "system/types.h"

namespace apic
{
enum delievery_modes
{
	DLM_FIXED = 0,
	DLM_LOWEST_PRIORITY = 1,
	DLM_SMI = 2,
	DLM_NMI = 4,
	DLM_INIT = 5,
	DLM_EXTINT = 7,
};

namespace local_apic
{

enum irq_destinations
{
	IRQDST_BROADCAST,
	IRQDST_BOOTSTRAP,
	IRQDEST_SINGLE,
};

enum apic_dest_shorthands
{
	APIC_DEST_SHORTHAND_NONE = 0x0,
	APIC_DEST_SHORTHAND_SELF = 0x1,
	APIC_DEST_SHORTHAND_ALL_AND_SELF = 0x2,
	APIC_DEST_SHORTHAND_ALL_BUT_SELF = 0x3,
};

enum apic_levels
{
	APIC_LEVEL_DEASSERT = 0x0,    /* 82489DX Obsolete. _Never_ use */
	APIC_LEVEL_ASSERT = 0x1,    /* Always use assert */
};

constexpr size_t ICRLO = 0x0300 / 4;
enum ICRLO_COMMAND
{
	ICRLO_CMD_INIT = 0x00000500,    // INIT/RESET
	ICRLO_CMD_STARTUP = 0x00000600, // Startup IPI
	ICRLO_CMD_DELIVS = 0x00001000,  // Delivery status
	ICRLO_CMD_ASSERT = 0x00004000,  // Assert interrupt (vs deassert)
	ICRLO_CMD_DEASSERT = 0x00000000,
	ICRLO_CMD_LEVEL = 0x00008000, // Level triggered
	ICRLO_CMD_BCAST = 0x00080000, // Send to all APICs, including self.
	ICRLO_CMD_BUSY = 0x00001000,
	ICRLO_CMD_FIXED = 0x00000000,
};

constexpr size_t ICRHI = 0x0310 / 4; //Interrupt Command [63:32]
constexpr size_t TIMER = 0x0320 / 4;
enum TIMER_FLAGS
{
	TIMER_FLAG_X1 = 0x0000000B,
	TIMER_FLAG_PERIODIC = 0x00020000,
};

constexpr size_t PCINT = 0x0340 / 4; // Performance Counter LVT
constexpr size_t LINT0 = 0x0350 / 4; // Local Vector Table 1 (LINT0)
constexpr size_t LINT1 = 0x0360 / 4; // Local Vector Table 2 (LINT1)
constexpr size_t ERROR = 0x0370 / 4; // Local Vector Table 3 (ERROR)

enum INTRRUPT_FLAGS
{
	INTERRUPT_MASKED = 0x00010000
};

constexpr size_t TICR = 0x0380 / 4;                  // Timer Initial Count
[[maybe_unused]] constexpr size_t TCCR = 0x0390 / 4; // Timer Current Count
constexpr size_t TDCR = 0x03E0 / 4;                  // Timer Divide Configuration
constexpr size_t TIC_DEFUALT_VALUE = 10000000;

extern volatile uint8_t* lapic_base;

PANIC void init_lapic();
size_t get_cpunum();
void write_eoi();

template<_internals::APICRegister T>
static inline T read_lapic(offset_t addr_off)
{
	return *((T * )(void * )(&local_apic::lapic_base[addr_off]));
}

template<_internals::APICLongRegister T>
static inline T read_lapic(offset_t addr_off)
{
	return *((T * )(void * )(&local_apic::lapic_base[addr_off]));
}

void write_lapic(offset_t addr_off, uint32_t value);

template<_internals::APICRegister T>
static inline void write_lapic(offset_t addr_off, T val)
{
	write_lapic(addr_off, *((uint32_t*)(void*)(&val)));
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