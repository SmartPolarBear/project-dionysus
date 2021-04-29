#pragma once

#include "system/types.h"

#include "ktl/concepts.hpp"

namespace apic::io_apic
{

template<typename T>
concept IOAPICRegister= sizeof(T) == sizeof(uint32_t) && std::is_standard_layout_v<T>;

template<typename T>
concept IOAPICLongRegister= sizeof(T) == sizeof(uint64_t) && std::is_standard_layout_v<T>;

union io_apic_redtbl_reg
{
	struct
	{
		uint64_t vector: 8;
		uint64_t delievery_mode: 3;
		uint64_t destination_mode: 1;
		uint64_t delivery_status: 1;
		uint64_t polarity: 1;
		uint64_t remote_irr: 1;
		uint64_t trigger_mode: 1;
		uint64_t mask: 1;
		uint64_t reserved: 39;
		uint64_t destination_id: 8;
	} __attribute__((packed));
	struct
	{
		uint32_t raw_low;
		uint32_t raw_high;
	} __attribute__((packed));
}__attribute__((packed));

static_assert(IOAPICLongRegister<io_apic_redtbl_reg>);

/// \brief This register contains the 4-bit APIC ID. The ID serves as a physical name of the IOAPIC. All APIC devices
/// using the APIC bus should have a unique APIC ID. The APIC bus arbitration ID for the I/O unit is also writtten
/// during a write to the APICID Register (same data is loaded into both). This register must be programmed with
/// the correct ID value before using the IOAPIC for message transmission.
struct io_apic_id_reg
{
	uint64_t res0: 24;
	uint64_t id: 4; //RW
	uint64_t res1: 4;
}__attribute__((packed));
static_assert(IOAPICRegister<io_apic_id_reg>);

struct io_apic_version_reg
{
	uint64_t version: 8;//RO
	uint64_t res0: 8;
	uint64_t max_redir_entries: 8;//RO
	uint64_t res1: 8;
}__attribute__((packed));
static_assert(IOAPICRegister<io_apic_version_reg>);

struct io_apic_arbitration_reg
{
	uint64_t res0: 24;
	uint64_t id: 4; //RW
	uint64_t res1: 4;
}__attribute__((packed));
static_assert(IOAPICRegister<io_apic_arbitration_reg>);

enum destination_mode
{
	DTM_PHYSICAL = 0,
	DTM_LOGICAL = 1,
};

enum trigger_mode
{
	TRG_EDGE = 0,
	TRG_LEVEL = 1,
};

enum ioapic_reg_addresses : offset_t
{
	IOAPICID = 0x00,
	IOAPICVER = 0x01,
	IOAPICARB = 0x02,
	IOREDTBL_BASE = 0x10,
};

}