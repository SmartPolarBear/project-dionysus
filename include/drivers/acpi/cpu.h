#pragma once
// #if !defined(__INCLUDE_DRIVERS_ACPI_CPU_H)
// #define __INCLUDE_DRIVERS_ACPI_CPU_H

#include "arch/amd64/cpu.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/segmentation.hpp"


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
	void* local_gs;

	task_state_segment tss{};
	gdt_table gdt_table{};

	cpu_struct()
		: id(0), apicid(0),
		  started(0), nest_pushcli_depth(0),
		  intr_enable(0), present(false),
		  local_fs(nullptr), local_gs(nullptr)
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

namespace ap
{
	void init_ap(void);
	void all_processor_main();
} // namespace ap

// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
