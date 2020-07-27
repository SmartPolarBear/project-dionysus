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

#include "system/error.h"
#include "system/kmalloc.h"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "arch/amd64/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

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
using lock::spinlock_initlock;
using lock::spinlock_release;
using lock::spinlock_holding;

// The design of the new vm manager:
// 1) When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// 2) provide an interface to dynamically map memory
// 3) handle the page fault so as to map pages on demand


mm_struct* vmm::mm_create(void)
{
	mm_struct* mm = reinterpret_cast<decltype(mm)>(memory::kmalloc(sizeof(mm_struct), 0));
	if (mm != nullptr)
	{
		list_init(&mm->vma_list);

		mm->mmap_cache = nullptr;
		mm->pgdir = nullptr;
		mm->map_count = 0;
	}

	spinlock_initlock(&mm->lock, "mm_lock");

	return mm;
}

error_code vmm::mm_map(IN mm_struct* mm, IN uintptr_t addr, IN size_t len, IN uint32_t vm_flags,
	OPTIONAL OUT vma_struct** vma_store)
{
	if (mm == nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	spinlock_acquire(&mm->lock);

	uintptr_t start = rounddown(addr, PAGE_SIZE), end = roundup(addr + len, PAGE_SIZE);
	if (!VALID_USER_REGION(start, end))
	{
		spinlock_release(&mm->lock);
		return -ERROR_INVALID_ADDR;
	}

	error_code ret = ERROR_SUCCESS;

	vma_struct* vma = nullptr;
	if ((vma = find_vma(mm, start)) != nullptr && end > vma->vm_start)
	{
		// the vma exists
		spinlock_release(&mm->lock);
		return ret;
	}
	else
	{
		vm_flags &= ~VM_SHARE;
		if ((vma = vma_create(start, end, vm_flags)) == nullptr)
		{
			spinlock_release(&mm->lock);
			return -ERROR_MEMORY_ALLOC;
		}

		insert_vma_struct(mm, vma);

		if (vma_store != nullptr)
		{
			*vma_store = vma;
		}
	}

	spinlock_release(&mm->lock);
	return ret;
}

error_code vmm::mm_unmap(IN mm_struct* mm, IN uintptr_t addr, IN size_t len)
{
	uintptr_t start = PAGE_ROUNDDOWN(addr), end = PAGE_ROUNDUP(addr + len);
	if (!VALID_USER_REGION(start, end))
	{
		return -ERROR_INVALID_ARG;
	}

	if (mm == nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	auto vma = find_vma(mm, start);
	if (vma == nullptr || end < vma->vm_start)
	{
		return ERROR_SUCCESS; // no need to remove
	}

	if (vma->vm_start < start && end < vma->vm_end)
	{
		//           range to remove
		//    [       [***********]       ]
		//     |------|      ^    |-------|
		//	  Create new     |     Shrink old
		//                   |
		//                 unmap

		auto new_vma = vma_create(vma->vm_start, start, vma->flags);
		if (new_vma == nullptr)
		{
			return -ERROR_MEMORY_ALLOC;
		}

		auto ret = vma_resize(vma, end, vma->vm_end);
		if (ret != ERROR_SUCCESS)
		{
			return ret;
		}

		insert_vma_struct(mm, new_vma);

		unmap_range(mm->pgdir, start, end);

		return ERROR_SUCCESS;
	}

	list_head free_list{};
	list_init(&free_list);

	for (list_head* iter = &vma->vma_link; iter != (&mm->vma_list); iter = iter->next)
	{
		auto iter_vma = list_entry(iter, vma_struct, vma_link);

		// vma list is sorted
		if (iter_vma->vm_start >= end)
		{
			break;
		}

		remove_vma(mm, iter_vma);
		list_add(&free_list, &(iter_vma->vma_link));
	}

	list_head* iter = nullptr;
	list_for(iter, &free_list)
	{
		auto iter_vma = list_entry(iter, vma_struct, vma_link);
		uintptr_t unmap_start = iter_vma->vm_start, unmap_end = iter_vma->vm_end;

		if (iter_vma->vm_start < start)
		{
			unmap_start = start;
			vma_resize(iter_vma, iter_vma->vm_start, start);
			insert_vma_struct(mm, iter_vma);
		}
		else
		{
			if (end < iter_vma->vm_end)
			{
				unmap_end = end;
				vma_resize(iter_vma, end, iter_vma->vm_end);
				insert_vma_struct(mm, iter_vma);
			}
			else
			{
				vma_destroy(iter_vma);
			}
		}
		unmap_range(mm->pgdir, unmap_start, unmap_end);
	}

	return ERROR_SUCCESS;
}

error_code vmm::mm_duplicate(IN mm_struct* to, IN const mm_struct* from)
{
	if (to != nullptr && from != nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	spinlock_acquire(&to->lock);
	list_head* iter = nullptr;
	list_for(iter, &from->vma_list)
	{
		auto vma = container_of(iter, vma_struct, vma_link);
		auto new_vma = vma_create(vma->vm_start, vma->vm_end, vma->flags);
		if (new_vma == nullptr)
		{
			spinlock_release(&to->lock);
			return -ERROR_MEMORY_ALLOC;
		}

		insert_vma_struct(to, new_vma);
		
		copy_range(from->pgdir, to->pgdir, vma->vm_start, vma->vm_end);
	}

	spinlock_release(&to->lock);
	return ERROR_SUCCESS;
}

void vmm::mm_destroy(mm_struct* mm)
{
	spinlock_acquire(&mm->lock);
	for (list_head* iter = mm->vma_list.next; iter != &mm->vma_list;)
	{
		auto next = iter->next;

		list_remove(iter);
		memory::kfree(container_of(iter, vma_struct, vma_link));

		iter = next;
	}

	spinlock_release(&mm->lock);
	memory::kfree(mm);
	mm = nullptr;
}

void vmm::mm_free(mm_struct* mm)
{
	KDEBUG_ASSERT(mm != nullptr && mm->map_count == 0);
	spinlock_acquire(&mm->lock);
	auto pgdir = mm->pgdir;

	list_head* iter = nullptr;

	list_for(iter, &mm->vma_list)
	{
		auto vma = container_of(iter, vma_struct, vma_link);
		unmap_range(pgdir, vma->vm_start, vma->vm_end);
	}

	iter = nullptr;
	list_for(iter, &mm->vma_list)
	{
		auto vma = container_of(iter, vma_struct, vma_link);
		free_range(pgdir, vma->vm_start, vma->vm_end);
	}

	spinlock_release(&mm->lock);
}

vma_struct* vmm::mm_intersect_vma(mm_struct* mm, uintptr_t start, uintptr_t end)
{
	auto vma = find_vma(mm, start);
	if (vma != nullptr && end <= vma->vm_start)
	{
		return nullptr;
	}
	return vma;
}

error_code vmm::mm_change_size(IN mm_struct* mm, uintptr_t addr, size_t len)
{
	uintptr_t start = PAGE_ROUNDDOWN(addr), end = PAGE_ROUNDUP((addr + len));
	if (!VALID_USER_REGION(start, end))
	{
		return -ERROR_INVALID_ARG;
	}

	error_code ret = ERROR_SUCCESS;

	if ((ret = mm_unmap(mm, start, end - start)) != ERROR_SUCCESS)
	{
		return ret;
	}

	constexpr auto VM_FLAGS = VM_READ | VM_WRITE;

	auto vma = find_vma(mm, start - 1);
	if (vma != nullptr && vma->vm_end == start && vma->flags == VM_FLAGS)
	{
		vma->vm_end = end;
		return ERROR_SUCCESS;
	}

	if ((vma = vma_create(start, end, VM_FLAGS)) == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	insert_vma_struct(mm, vma);

	return ERROR_SUCCESS;
}

