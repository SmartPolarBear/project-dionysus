/*
 * Last Modified: Sun May 17 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#include "system/error.h"
#include "system/kmalloc.h"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "arch/amd64/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include <algorithm>
#include <cstring>

using vmm::mm_struct;
using vmm::pde_ptr_t;
using vmm::pde_t;
using vmm::vma_struct;

using pmm::boot_mem::boot_alloc_page;

// linked list
using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;


// cpu-individual variable containing info about current CPU
#ifndef USE_NEW_CPU_INTERFACE
__thread cpu_struct *cpu;
#endif

static inline void set_gdt_entry(OUT
	gdt_entry* entry,
	uintptr_t base,
	size_t limit,
	dpl_values dpl,
	bool executable,
	bool rw)
{
	gdt_access_byte_struct access_byte{};
	gdt_flags_struct flags{};

	access_byte.pr = 1;
	access_byte.privl = ((uint8_t)dpl);
	access_byte.s = 1;
	access_byte.rw = rw;
	access_byte.ex = executable;

	flags.l = executable;// L bit for x86-64

//	uint8_t access_byte = 0;
//	uint8_t flags = 0;
//
//	access_byte |= 0b10000000u; // present
//	access_byte |= ((uint8_t)(((uint8_t)dpl) << 5u)); // dpl
//	access_byte |= 0b00010000u; // code or data segment
//
//	if (rw)
//	{
//		access_byte |= 0b00000010u;
//	}
//
//	if (executable)
//	{
//		access_byte |= 0b00001000u;
//	}
//
//	if (executable)
//	{
//		flags |= 0b0010u; // L bit for x86-64
//	}

	entry->flags = gdt_flags_to_int(flags);
	entry->access_byte = gdt_access_byte_to_int(access_byte);

	entry->base_low = base & 0xFFFFu;
	entry->base_mid = (base >> 16u) & 0xFFu;
	entry->base_high = (base >> 24u) & 0xFFu;

	entry->limit_low = limit & 0xFFFFu;
	entry->limit_low = (limit >> 16u) & 0xFu;

}

void vmm::install_gdt()
{
	auto current_cpu = &cpus[local_apic::get_cpunum()];

	uint8_t* cpu_fs = reinterpret_cast<decltype(cpu_fs)>(boot_alloc_page());

	if (cpu_fs == nullptr)
	{
		KDEBUG_FOLLOWPANIC("Cannot allocate memory for kernel GS\n");
	}

	memset(cpu_fs, 0, PAGE_SIZE);
	wrmsr(MSR_FS_BASE, ((uintptr_t)cpu_fs));
	current_cpu->local_fs = cpu_fs;

	uint8_t* cpu_kernel_gs = reinterpret_cast<decltype(cpu_kernel_gs)>(boot_alloc_page());

	if (cpu_kernel_gs == nullptr)
	{
		KDEBUG_FOLLOWPANIC("Cannot allocate memory for kernel GS\n");
	}

	memset(cpu_kernel_gs, 0, PAGE_SIZE);
	wrmsr(MSR_KERNEL_GS_BASE, ((uintptr_t)cpu_kernel_gs));

	current_cpu->local_gs = cpu_kernel_gs;

	current_cpu->tss.iopb_offset = sizeof(current_cpu->tss);

	auto s2 = sizeof(gdt_entry);

	set_gdt_entry(&current_cpu->gdt_table.kernel_code, 0, 0, DPL_KERNEL, true, false);
	set_gdt_entry(&current_cpu->gdt_table.kernel_data, 0, 0, DPL_KERNEL, false, true);
	set_gdt_entry(&current_cpu->gdt_table.user_code, 0, 0, DPL_USER, true, false);
	set_gdt_entry(&current_cpu->gdt_table.user_data, 0, 0, DPL_USER, false, true);

	KDEBUG_ASSERT((*((uint64_t*)(&current_cpu->gdt_table.kernel_code))) == 0x0020980000000000ul);  // Code, DPL=0, R/X
	KDEBUG_ASSERT((*((uint64_t*)(&current_cpu->gdt_table.kernel_data))) == 0x0000920000000000ul);  // Data, DPL=0, W
	KDEBUG_ASSERT((*((uint64_t*)(&current_cpu->gdt_table.user_code))) == 0x0020F80000000000ul);  // Code, DPL=3, R/X
	KDEBUG_ASSERT((*((uint64_t*)(&current_cpu->gdt_table.user_data))) == 0x0000F20000000000ul);  // Data, DPL=3, W

	uintptr_t tss_addr = (uintptr_t)(&current_cpu->tss);
	(*((uint64_t*)(&current_cpu->gdt_table.tss_low))) = (0x0067ul) | ((tss_addr & 0xFFFFFFul) << 16ul) |
		(0x00E9ul << 40ul) | (((tss_addr >> 24ul) & 0xFFul) << 56ul);
	(*((uint64_t*)(&current_cpu->gdt_table.tss_high))) = (tss_addr >> 32ul);

	current_cpu->install_gdt_and_tss();

// --target=x86_64-pc-none-elf and -mcmodel=large can cause a triple fault here
// work it around by building with x86_64-pc-linux-elf
#ifndef USE_NEW_CPU_INTERFACE
	cpu = current_cpu;
#else
	cls_put(CLS_CPU_STRUCT_PTR, current_cpu);
#endif
}
