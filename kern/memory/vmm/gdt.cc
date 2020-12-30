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

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include <algorithm>
#include <cstring>

using vmm::mm_struct;
using vmm::pde_ptr_t;
using vmm::pde_t;
using vmm::vma_struct;

using pmm::boot_mem::boot_alloc_page;

// linked list
using kbl::list_add;
using kbl::list_empty;
using kbl::list_for_each;
using kbl::list_init;
using kbl::list_remove;

// cpu-individual variable containing info about current CPU
CLSItem<cpu_struct*, CLS_CPU_STRUCT_PTR> cpu{ false };

static inline void set_gdt_entry(OUT
	gdt_entry* entry,
	uintptr_t base,
	size_t limit,
	dpl_values dpl,
	bool executable,
	bool rw)
{
	gdt_access_byte_struct access_byte{};

	access_byte.pr = 1;
	access_byte.privl = ((uint8_t)dpl);
	access_byte.s = 1;
	access_byte.rw = rw;
	access_byte.ex = executable;

	entry->access_byte = gdt_access_byte_to_int(access_byte);

	gdt_flags_struct flags{};

	flags.l = executable;// L bit for x86-64

	entry->flags = gdt_flags_to_int(flags);

	entry->base_low = base & 0xFFFFu;
	entry->base_mid = (base >> 16u) & 0xFFu;
	entry->base_high = (base >> 24u) & 0xFFu;

	entry->limit_low = limit & 0xFFFFu;
	entry->limit_high = (limit >> 16u) & 0xFu;
}

static inline void set_tss_gdt_entry(gdt_entry* entry_low, gdt_entry* entry_high, uintptr_t tss_addr)
{
	entry_low->access_byte = 0xE9u; //TSS specified access byte

	gdt_flags_struct flags{};
	flags.l = true;// L bit for x86-64
	entry_low->flags = 0x0;

	entry_low->base_low = tss_addr & 0xFFFFu;
	entry_low->base_mid = (tss_addr >> 16u) & 0xFFu;
	entry_low->base_high = (tss_addr >> 24u) & 0xFFu;

	size_t tss_limit = sizeof(task_state_segment) - 1;
	entry_low->limit_low = (tss_limit & 0xFFFFu);
	entry_low->limit_high = ((tss_limit >> 16u) & 0xFu);

	// tss-high isn't structured as usual
	(*((uint64_t*)entry_high)) = (tss_addr >> 32ul);
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

	current_cpu->kernel_gs = cpu_kernel_gs;

	current_cpu->tss.iopb_offset = sizeof(current_cpu->tss);

	set_gdt_entry(&current_cpu->gdt_table.kernel_code, 0, 0, DPL_KERNEL, true, false);
	set_gdt_entry(&current_cpu->gdt_table.kernel_data, 0, 0, DPL_KERNEL, false, true);
	set_gdt_entry(&current_cpu->gdt_table.user_code, 0, 0, DPL_USER, true, false);
	set_gdt_entry(&current_cpu->gdt_table.user_data, 0, 0, DPL_USER, false, true);

	set_tss_gdt_entry(&current_cpu->gdt_table.tss_low, &current_cpu->gdt_table.tss_high, (uintptr_t)&current_cpu->tss);

	current_cpu->install_gdt_and_tss();

// --target=x86_64-pc-none-elf and -mcmodel=large can cause a triple fault here
// work it around by building with x86_64-pc-linux-elf
	cpu = current_cpu;
}
