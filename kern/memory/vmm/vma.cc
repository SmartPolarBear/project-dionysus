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
using kbl::list_add;
using kbl::list_empty;
using kbl::list_for_each;
using kbl::list_init;
using kbl::list_remove;

// spinlock_struct
using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

// The design of the new vm manager:
// 1) When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// 2) provide an interface to dynamically map memory
// 3) handle the page fault so as to map pages on demand

void check_vma_overlap(vma_struct* prev, vma_struct* next)
{
	KDEBUG_ASSERT(prev->vm_start < prev->vm_end);
	KDEBUG_ASSERT(prev->vm_end <= next->vm_start);
	KDEBUG_ASSERT(next->vm_start < next->vm_end);
}

void remove_vma(mm_struct* mm, vma_struct* vma)
{
	list_remove(&vma->vma_link);
	mm->map_count--;
}

vma_struct* vmm::find_vma(mm_struct* mm, uintptr_t addr)
{
	KDEBUG_ASSERT(mm != nullptr);
	vma_struct* ret = mm->mmap_cache;
	if (!(ret != nullptr &&
		ret->vm_start <= addr &&
		ret->vm_end > addr))
	{
		ret = nullptr;
		list_head* entry = nullptr;
		list_for(entry, &mm->vma_list)
		{
			auto vma = container_of(entry, vma_struct, vma_link);
			if (vma->vm_start <= addr && addr < vma->vm_end)
			{
				ret = vma;
				break;
			}
		}
	}

	if (ret != nullptr)
	{
		mm->mmap_cache = ret;
	}

	return ret;
}

vma_struct* vmm::vma_create(uintptr_t vm_start, uintptr_t vm_end, size_t vm_flags)
{
	vma_struct* vma = reinterpret_cast<decltype(vma)>(memory::kmalloc(sizeof(vma_struct), 0));

	if (vma != nullptr)
	{
		vma->vm_start = vm_start;
		vma->vm_end = vm_end;
		vma->flags = vm_flags;
	}

	return vma;
}

void vmm::insert_vma_struct(mm_struct* mm, vma_struct* vma)
{
	KDEBUG_ASSERT(vma->vm_start < vma->vm_end);

	// find the place to insert to
	list_head* prev = &mm->vma_list;

	list_head* iter = nullptr;
	list_for(iter, &mm->vma_list)
	{
		auto prev_vma = container_of(iter, vma_struct, vma_link);
		if (prev_vma->vm_start > vma->vm_start)
		{
			break;
		}
		prev = iter;
	}

	auto next = prev->next;

	if (prev != &mm->vma_list)
	{
		check_vma_overlap(container_of(prev, vma_struct, vma_link), vma);
	}

	if (next != &mm->vma_list)
	{
		check_vma_overlap(vma, container_of(next, vma_struct, vma_link));
	}

	vma->mm = mm;
	list_add(&vma->vma_link, prev);

	mm->map_count++;
}

error_code vmm::vma_resize(vma_struct* vma, uintptr_t start, uintptr_t end)
{
	if ((start % PAGE_SIZE) != 0 || (end % PAGE_SIZE) != 0)
	{
		return -ERROR_INVALID;
	}

	if (start >= end)
	{
		return -ERROR_INVALID;
	}

	if (!(vma->vm_start <= start && end <= vma->vm_end))
	{
		return -ERROR_INVALID;
	}

	vma->vm_start = start;
	vma->vm_end = end;

	return ERROR_SUCCESS;
}

void vmm::vma_destroy(vma_struct* vma)
{
	memory::kfree(vma);
}


