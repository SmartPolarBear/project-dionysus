#pragma once

#include "arch/amd64/cpu/cpu.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/segmentation.hpp"

#include "system/cls.hpp"

struct context
{
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip; //rip will be pop automatically by ret instruction
}__attribute__((packed));

struct cpu_struct
{
	uint8_t id;                // index into cpus[] below
	uint8_t apicid;            // Local APIC ID
	volatile uint32_t started; // Has the CPU started?
	int nest_pushcli_depth;    // Depth of pushcli nesting.
	int intr_enable;           // Were interrupts enabled before pushcli?
	bool present;              // Is this core available

	// Cpu-local storage variables
	void* local_fs;
	void* kernel_gs;

	// scheduler context
	::context* scheduler;

	task_state_segment tss{};
	gdt_table gdt_table{};

	cpu_struct()
		: id(0), apicid(0),
		  started(0), nest_pushcli_depth(0),
		  intr_enable(0), present(false),
		  local_fs(nullptr), kernel_gs(nullptr),
		  scheduler(nullptr)
	{

	}

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

extern CLSItem<cpu_struct*, CLS_CPU_STRUCT_PTR> cpu;

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"

template<typename T>
static inline T gs_get_cpu_dependent(uintptr_t n)
requires ktl::Pointer<T>
{
	uintptr_t* target_gs_ptr = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + n);
	uintptr_t ret = *target_gs_ptr;
	return (T)(void*)ret;
}

template<typename T>
static inline void gs_put_cpu_dependent(uintptr_t n, T v)
requires ktl::Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	uintptr_t* target_gs_ptr = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + n);
	*target_gs_ptr = val;
}

#pragma clang diagnostic pop

namespace ap
{
	void init_ap(void);

	// in main.cc
	void all_processor_main();
} // namespace ap

// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
