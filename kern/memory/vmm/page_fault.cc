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

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"

#include "arch/amd64/x86.h"

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

static inline error_code page_fault_impl(mm_struct* mm, size_t err, uintptr_t addr)
{
	vma_struct* vma = vmm::find_vma(mm, addr);
	if (vma == nullptr || vma->vm_start > addr)
	{
		return -ERROR_VMA_NOT_FOUND;
	}
	else
	{

		switch (err & 0b11)
		{
		default:
		case 0b10: // write, not persent
			if (!(vma->flags & vmm::VM_WRITE))
			{
				return -ERROR_PAGE_NOT_PRESENT;
			}
			break;
		case 0b01: // read, persent
			return -ERROR_UNKOWN;
			break;
		case 0b00: // read not persent
			if (!(vma->flags & (vmm::VM_READ | vmm::VM_EXEC)))
			{
				return -ERROR_PAGE_NOT_PRESENT;
			}
			break;
		}

		size_t page_perm = PG_U;
		if (vma->flags & vmm::VM_WRITE)
		{
			page_perm |= PG_W;
		}

		addr = rounddown(addr, PAGE_SIZE);

		page_info* page_ret = nullptr;
		if (pmm::pgdir_alloc_page(mm->pgdir, true, addr, page_perm, &page_ret)
			!= ERROR_SUCCESS) // map to any free space
		{
			return -ERROR_MEMORY_ALLOC;
		}
		return ERROR_SUCCESS;
	}
}

error_code handle_pgfault([[maybe_unused]] trap::trap_frame info)
{
	uintptr_t addr = rcr2();
	mm_struct* mm = cur_proc != nullptr ? cur_proc->get_mm() : nullptr;

	KDEBUG_ASSERT(cur_proc != nullptr && cur_proc->get_mm() != nullptr);

	// The address belongs to the kernel.
	if (mm == nullptr)
	{
		KDEBUG_RICHPANIC("Unkown error in paging",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}

	error_code ret = page_fault_impl(mm, info.err, addr);

	if (ret == -ERROR_VMA_NOT_FOUND)
	{
		KDEBUG_RICHPANIC("The addr isn't found in th MM structure.",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}
	else if (ret == -ERROR_PAGE_NOT_PRESENT)
	{
		KDEBUG_RICHPANIC("A page's not persent.",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}
	else if (ret == -ERROR_UNKOWN)
	{
		KDEBUG_RICHPANIC("Unkown error in paging",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}

	return ret;
}