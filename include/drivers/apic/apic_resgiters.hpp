#pragma once

#include "system/types.h"

#include "ktl/concepts.hpp"

namespace apic
{

namespace _internals
{
template<typename T>
concept MSRRegister = sizeof(T) == sizeof(uint64_t) && std::is_standard_layout_v<T>;

template<typename T>
concept APICRegister= sizeof(T) == sizeof(uint32_t) && std::is_standard_layout_v<T>;

template<typename T>
concept APICLongRegister= sizeof(T) == sizeof(uint64_t) && std::is_standard_layout_v<T>;

struct apic_base_msr_reg
{
	uint64_t res0_: 8;
	uint64_t bso_: 1;
	uint64_t res1_: 2;
	uint64_t global_enable_: 1;
	uint64_t base_: 24;
	uint64_t res2_: 28;
}__attribute__((packed));
static_assert(MSRRegister<apic_base_msr_reg>);

union lapic_priority_reg
{
	struct
	{
		uint64_t p_subclass: 4;
		uint64_t p_class: 4;
		uint64_t res0: 24;
	}__attribute__((packed));
	uint32_t raw;
}__attribute__((packed));
static_assert(APICRegister<lapic_priority_reg>);

using lapic_process_priority_reg = lapic_priority_reg;
using lapic_task_priority_reg = lapic_priority_reg;

struct lapic_id_reg
{
	uint64_t res0: 24;
	uint64_t apic_id: 8;
}__attribute__((packed));
static_assert(APICRegister<lapic_id_reg>);

struct lapic_version_reg
{
	uint64_t version: 8;
	uint64_t res0: 8;
	uint64_t max_lvt: 8;
	uint64_t eoi_broadcast_suppression: 1;
	uint64_t res1: 7;
}__attribute__((packed));
static_assert(APICRegister<lapic_version_reg>);

struct lvt_timer_reg
{
	uint64_t vector: 8;
	uint64_t res0: 4;
	uint64_t delivery_status: 1;
	uint64_t res1: 3;
	uint64_t mask: 1;
	uint64_t timer_mode: 2;
	uint64_t res3: 13;
}__attribute__((packed));
static_assert(APICRegister<lvt_timer_reg>);

union lvt_error_reg
{
	struct
	{
		uint64_t vector: 8;
		uint64_t res0: 4;
		uint64_t delivery_status: 1;
		uint64_t res1: 3;
		uint64_t mask: 1;
		uint64_t res2: 15;
	}__attribute__((packed));
	uint32_t raw;
}__attribute__((packed));
static_assert(APICRegister<lvt_error_reg>);

union lvt_lint_reg
{
	struct
	{
		uint64_t vector: 8;
		uint64_t delivery_mode: 3;
		uint64_t res0: 1;
		uint64_t delivery_status: 1;
		uint64_t input_pin_polarity: 1;
		uint64_t remote_irr: 1;
		uint64_t trigger_mode: 1;
		uint64_t masked: 1;
		uint64_t res1: 15;
	}__attribute__((packed));
	uint32_t raw;
}__attribute__((packed));
static_assert(APICRegister<lvt_lint_reg>);

union lvt_common_reg
{
	struct
	{
		uint64_t vector: 8;
		uint64_t delivery_mode: 3;
		uint64_t res0: 1;
		uint64_t delivery_status: 1;
		uint64_t res1: 3;
		uint64_t mask: 1;
		uint64_t res2: 15;
	}__attribute__((packed));
	uint32_t raw;
}__attribute__((packed));
static_assert(APICRegister<lvt_common_reg>);

using lvt_cmci_reg = lvt_common_reg;
using lvt_perf_mon_counters_reg = lvt_common_reg;
using lvm_thermal_sensor_reg = lvt_common_reg;

struct error_status_reg
{
	uint64_t error_bits: 8;
	uint64_t res0: 24;
}__attribute__((packed));
static_assert(APICRegister<error_status_reg>);

struct timer_divide_configuration_reg
{
	uint64_t divide_val: 4;
	uint64_t res0: 28;
}__attribute__((packed));
static_assert(APICRegister<timer_divide_configuration_reg>);

using initial_count_reg = uint32_t;
using current_count_reg = uint32_t;

using eoi_reg = uint32_t;

union svr_reg
{
	struct
	{
		uint64_t vector: 8;
		uint64_t apic_software_enable: 1;
		uint64_t focus_proc_checking: 1;
		uint64_t res0: 2;
		uint64_t eoi_broadcast_suppression: 1;
		uint64_t res1: 19;
	}__attribute__((packed));
	uint32_t raw;
}__attribute__((packed));

static_assert(APICRegister<svr_reg>);

union lapic_icr_reg
{
	struct
	{
		uint32_t vector: 8, delivery_mode: 3, dest_mode: 1,
			delivery_status: 1, reserved0: 1, level: 1,
			trigger: 1, reserved1: 2, dest_shorthand: 2,
			reserved2: 12;
		uint32_t reserved3: 24, dest: 8;
	} __attribute__((__packed__));

	struct
	{
		uint32_t value_low;
		uint32_t value_high;
	} __attribute__((__packed__));

	uint64_t value;
}__attribute__((__packed__));

static_assert(APICLongRegister<lapic_icr_reg>);

}

enum [[clang::enum_extensibility(closed)]] timer_divide_values
{
	TIMER_DIV1 = 0B1011,
	TIMER_DIV2 = 0B0000,
	TIMER_DIV4 = 0B1000,
	TIMER_DIV8 = 0B0010,
	TIMER_DIV16 = 0B0011,
	TIMER_DIV32 = 0B0001,
	TIMER_DIV64 = 0B1001,
	TIMER_DIV128 = 0B0011

};

enum [[clang::flag_enum]] esr_error_bits
{
	ESR_SEND_CHECKSUM_ERROR = 1,
	ESR_RECEIVE_CHECKSUM_ERROR = 1 << 1,
	ESR_SEND_ACCEPT_ERROR = 1 << 2,
	ESR_RECEIVE_ACCEPT_ERROR = 1 << 3,
	ESR_REDIRECTABLE_IPI = 1 << 4,
	ESR_SEND_ILLEGAL_VEC = 1 << 5,
	ESR_RECEIVE_ILLEGAL_VEC = 1 << 6,
	ESR_ILLEGAL_REG_ADDR = 1 << 7
};

enum register_addresses
{
	ID_ADDR = 0x020,
	VERSION_ADDR = 0x030,
	TASK_PRI_ADDR = 0x080,
	PROC_PRI_ADDR = 0x0A0,
	EOI_ADDR = 0x0B0,
	LVT_TIMER_ADDR = 0x320,
	LVT_PC_ADDR = 0x340,
	DCR_ADDR = 0x3E0,
	INITIAL_COUNT_ADDR = 0x380,
	CURRENT_COUNT_ADDR = 0x390,
	LINT0_ADDR = 0x350,
	LINT1_ADDR = 0x360,
	ERROR_ADDR = 0x370,
	ESR_ADDR = 0x280,
	SVR_ADDR = 0xF0,
	ICR_LO_ADDR = 0x300,
	ICR_HI_ADDR = 0x310
};

enum apic_timer_modes
{
	TIMER_ONE_SHOT = 0b00,
	TIMER_PERIODIC = 0b01,
	TIMER_TSC_DEADLINE = 0b10,
	TIMER_RESERVED = 0b11
};

enum delievery_modes
{
	DLM_FIXED = 0,
	DLM_LOWEST_PRIORITY = 1,
	DLM_SMI = 2,
	DLM_NMI = 4,
	DLM_INIT = 5,
	DLM_STARTUP = 0b110,
	DLM_EXTINT = 7,
};

enum delivery_statuses : uint32_t
{
	DLS_IDLE = 0,
	DLS_SEND_PENDING = 1,
};

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

template<typename T>
class i_apic_register
{
 public:
	virtual T& load() = 0;
	virtual T& clear() = 0;
	virtual void apply() = 0;
};

}