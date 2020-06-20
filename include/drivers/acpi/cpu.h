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

	task_state_segment tss;
	gdt_table gdt_table;

	cpu_struct()
		: id(0), apicid(0),
		  started(0), nest_pushcli_depth(0),
		  intr_enable(0), present(false),
		  local_fs(nullptr)
	{
		gdt_table = {
			{ 0, 0, 0, 0x00, 0x00, 0 }, /* 0x00 null  */
			{ 0, 0, 0, 0x9a, 0xa0, 0 }, /* 0x08 kernel code (kernel base selector) */
			{ 0, 0, 0, 0x92, 0xa0, 0 }, /* 0x10 kernel data */
			{ 0, 0, 0, 0x00, 0x00, 0 }, /* 0x18 null (user base selector) */
			{ 0, 0, 0, 0x92, 0xa0, 0 }, /* 0x20 user data */
			{ 0, 0, 0, 0x9a, 0xa0, 0 }, /* 0x28 user code */
			{ 0, 0, 0, 0x92, 0xa0, 0 }, /* 0x30 ovmf data */
			{ 0, 0, 0, 0x9a, 0xa0, 0 }, /* 0x38 ovmf code */
			{ 0, 0, 0, 0x89, 0xa0, 0 }, /* 0x40 tss low */
			{ 0, 0, 0, 0x00, 0x00, 0 }, /* 0x48 tss high */
		};

//		*((uint64_t*)(&gdt_table.kernel_code)) = 0x0020980000000000;
//		*((uint64_t*)(&gdt_table.kernel_data)) = 0x0000920000000000;
//		*((uint64_t*)(&gdt_table.user_code)) = 0x0020F80000000000;
//		*((uint64_t*)(&gdt_table.user_data)) = 0x0000F20000000000;
	}

	void install_gdt_and_tss()
	{
		gdt_table_desc gdt_desc = { sizeof(gdt_table) - 1, (uintptr_t)&gdt_table };
		load_gdt_and_tr(&gdt_desc, SEGMENTSEL_TSSLOW);
	}
};

constexpr size_t CPU_COUNT_LIMIT = 8;

extern cpu_struct cpus[CPU_COUNT_LIMIT];
extern uint8_t cpu_count;

#define USE_NEW_CPU_INTERFACE
#ifndef USE_NEW_CPU_INTERFACE
extern __thread cpu_struct *cpu;
#else
__attribute__((always_inline)) static inline cpu_struct* cpu()
{
	return cls_get<cpu_struct*>(CLS_CPU_STRUCT_PTR);
}
#endif

namespace ap
{
	void init_ap(void);
	void all_processor_main();
} // namespace ap

// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
