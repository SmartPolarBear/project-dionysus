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

struct apic_base_msr_struct
{
	uint64_t res0_: 8;
	uint64_t bso_: 1;
	uint64_t res1_: 2;
	uint64_t global_enable_: 1;
	uint64_t base_: 24;
	uint64_t res2_: 28;
}__attribute__((packed));
static_assert(MSRRegister<apic_base_msr_struct>);

struct lapic_id_struct
{
	uint64_t res0: 24;
	uint64_t apic_id: 8;
}__attribute__((packed));
static_assert(APICRegister<lapic_id_struct>);

struct lapic_version_struct
{
	uint64_t version: 8;
	uint64_t res0: 8;
	uint64_t max_lvt: 8;
	uint64_t eoi_broadcast_suppression: 1;
	uint64_t res1: 7;
}__attribute__((packed));
static_assert(APICRegister<lapic_version_struct>);

}

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
		_internals::apic_base_msr_struct msr;
		uint64_t raw_;
	};
}__attribute__((packed));

static_assert(_internals::MSRRegister<apic_base_msr>);
}