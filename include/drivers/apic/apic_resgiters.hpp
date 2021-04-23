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

struct lvt_error_reg
{
	uint64_t vector: 8;
	uint64_t res0: 4;
	uint64_t delivery_status: 1;
	uint64_t res1: 3;
	uint64_t mask: 1;
	uint64_t res2: 15;
}__attribute__((packed));
static_assert(APICRegister<lvt_error_reg>);

struct lvt_lint_reg
{
	uint64_t vector: 8;
	uint64_t delivery_mode: 3;
	uint64_t res0: 1;
	uint64_t delivery_status: 1;
	uint64_t input_pin_polarity: 1;
	uint64_t remote_irr: 1;
	uint64_t trigger_mode: 1;
	uint64_t mask: 1;
	uint64_t res1: 15;
}__attribute__((packed));
static_assert(APICRegister<lvt_lint_reg>);

struct lvt_common_reg
{
	uint64_t vector: 8;
	uint64_t delivery_mode: 3;
	uint64_t res0: 1;
	uint64_t delivery_status: 1;
	uint64_t res1: 3;
	uint64_t mask: 1;
	uint64_t res2: 15;
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

}

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

class apic_base_msr
{
 public:
	apic_base_msr(uint64_t raw) : raw_(raw)
	{
	}

	~apic_base_msr() = default;

	apic_base_msr(const apic_base_msr& another) : raw_(another.raw_)
	{

	}

	apic_base_msr(apic_base_msr&& another) : raw_(std::exchange(another.raw_, 0))
	{

	}

	apic_base_msr& operator=(const apic_base_msr& another)
	{
		raw_ = another.raw_;
		return *this;
	}

 private:
	union
	{
		_internals::apic_base_msr_reg msr;
		uint64_t raw_;
	};
}__attribute__((packed));

static_assert(_internals::MSRRegister<apic_base_msr>);
}