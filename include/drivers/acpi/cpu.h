#pragma once

#include "arch/amd64/cpu/cpu.h"

#include "system/types.h"
#include "system/segmentation.hpp"

#include "system/cls.hpp"
#include "system/dpc.hpp"

#include "task/scheduler/scheduler.hpp"

using context = arch_task_context_registers;

struct cpu_struct
{
	uint8_t id{ 0 };                // index into cpus[] below
	uint8_t apicid{ 0 };            // Local APIC ID
	volatile uint32_t started{ 0 }; // Has the CPU started?
	int nest_pushcli_depth{ 0 };    // Depth of pushcli nesting.
	int intr_enable{ false };           // Were interrupts enabled before pushcli?
	bool present{ false };              // Is this core available

	// Cpu-local storage variables
	void* local_fs{ nullptr };
	void* kernel_gs{ nullptr };

	task::thread *idle;
	task::scheduler scheduler{};

	task_state_segment tss{};
	gdt_table gdt_table{};

	cpu_struct() = default;

	void install_gdt_and_tss()
	{
		gdt_table_desc gdt_desc{ sizeof(gdt_table) - 1, (uintptr_t)&gdt_table };

		load_gdt(&gdt_desc);

		load_tr(SEGMENTSEL_TSSLOW);
	}
};

constexpr size_t CPU_COUNT_LIMIT = 8;

extern cpu_struct cpus[CPU_COUNT_LIMIT];
extern uint8_t cpu_count;

extern cls_item<cpu_struct*, CLS_CPU_STRUCT_PTR> cpu;

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"

template<typename T>
requires ktl::Convertible<T, uintptr_t>
static inline T gs_get_cpu_dependent(uintptr_t n)
{
	uintptr_t* target_gs_ptr = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + n);
	uintptr_t ret = *target_gs_ptr;
	return (T)(void*)ret;
}

template<typename T>
requires ktl::Convertible<T, uintptr_t>
static inline void gs_put_cpu_dependent(uintptr_t n, T v)
{
	uintptr_t val = (uintptr_t)v;
	uintptr_t* target_gs_ptr = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + n);
	*target_gs_ptr = val;
}

#pragma clang diagnostic pop


// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
