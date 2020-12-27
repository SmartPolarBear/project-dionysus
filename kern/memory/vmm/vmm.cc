/*
 * Last Modified: Sun May 10 2020
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

#include "vmm.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include <cstring>
#include <algorithm>

using vmm::mm_struct;
using vmm::pde_ptr_t;
using vmm::pde_t;
using vmm::vma_struct;

using pmm::boot_mem::boot_alloc_page;

// linked list
using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

// spinlock
using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

// The design of the new vm manager:
// 1) When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// 2) provide an interface to dynamically map memory
// 3) handle the page fault so as to map pages on demand

void vmm::init_vmm(void)
{
	// create the global pml4t
	pgdir_cache_init();

	g_kpml4t = pgdir_entry_alloc();

	memset(g_kpml4t, 0, PGTABLE_SIZE);

	// register the page fault handle
	trap::trap_handle_register(trap::TRAP_PGFLT, trap::trap_handle{
		.handle = handle_pgfault,
		.enable = true });
}

bool vmm::check_user_memory(IN mm_struct* mm, uintptr_t addr, size_t len, bool writable)
{
	if (mm != nullptr)
	{
		if (!VALID_USER_REGION(addr, addr + len))
		{
			return false;
		}

		vma_struct* vma = nullptr;
		for (uintptr_t start = addr, end = addr + len;
			 start < end;
			 start = vma->vm_end)
		{
			if ((vma = find_vma(mm, start)) == nullptr || start < vma->vm_start)
			{
				return false;
			}
			if (!(vma->flags & ((writable) ? VM_WRITE : VM_READ)))
			{
				return false;
			}
			if (writable && (vma->flags & VM_STACK))
			{
				if (start < vma->vm_start + PAGE_SIZE)
				{
					return false;
				}
			}
		}
		return true;
	}

	return VALID_KERNEL_REGION(addr, addr + len);
}
