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

#pragma once

#include "system/types.h"

#include "kbl/lock/spinlock.h"

namespace vmm
{
	using pde_t = size_t;
	using pde_ptr_t = pde_t*;

	struct vma_struct;

	struct mm_struct
	{
		// TODO: optimize it with trees
		list_head vma_list;    // linked list of vma structures
		vma_struct* mmap_cache; // for quicker search of vma
		pde_ptr_t pgdir;
		size_t map_count;
		uintptr_t brk_start, brk;

		lock::spinlock_struct lock;
	};

	struct vma_struct
	{
		mm_struct* mm; // the memory this struct belongs to
		uintptr_t vm_start;
		uintptr_t vm_end;
		size_t flags;
		// TODO: optimize it with trees
		list_head vma_link;
	};

	enum VM_FLAGS : size_t
	{
		VM_READ = 0x00000001,
		VM_WRITE = 0x00000002,
		VM_EXEC = 0x00000004,
		VM_STACK = 0x00000008,
		VM_SHARE = 0x00000010,
	};

	// paging.cc
	extern vmm::pde_ptr_t g_kpml4t;

	vmm::pde_ptr_t pgdir_entry_alloc();

	void pgdir_entry_free(vmm::pde_ptr_t entry);

	vma_struct* find_vma(mm_struct* mm, uintptr_t addr);

	vma_struct* vma_create(uintptr_t vm_start, uintptr_t vm_end, size_t vm_flags);

	error_code vma_resize(vma_struct* vma, uintptr_t start, uintptr_t end);

	void vma_destroy(vma_struct *vma);

	void insert_vma_struct(mm_struct* mm, vma_struct* vma);

	mm_struct* mm_create(void);

	error_code mm_map(IN mm_struct* mm, IN uintptr_t addr, IN size_t len, IN uint32_t vm_flags,
		OPTIONAL OUT vma_struct** vma_store);

	error_code mm_unmap(IN mm_struct* mm, IN uintptr_t addr, IN size_t len);

	error_code mm_duplicate(IN mm_struct* to, IN const mm_struct* from);

	error_code mm_change_size(IN mm_struct* mm, uintptr_t addr, size_t len);

	vma_struct* mm_intersect_vma(IN mm_struct* mm, uintptr_t start, uintptr_t end);

	bool check_user_memory(IN mm_struct* mm, uintptr_t addr, size_t len, bool writable);

	void mm_destroy(mm_struct* mm);

	void mm_free(mm_struct* mm);

// initialize the vmm
// 1) register the page fault handle
// 2) allocate an pml4 table
	void init_vmm(void);

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// and then map all the memories to PHYREMAP_VIRTUALBASE
	void paging_init(void);

// install GDT
	void install_gdt(void);

// install g_kml4_t to cr3
	void install_kernel_pml4t(void);

	// get a copy of the kernel pml4t

	void duplicate_kernel_pml4t(OUT pde_ptr_t pml4t);

// get the physical address mapped by a pde
	uintptr_t pde_to_pa(pde_ptr_t pde);

	pde_ptr_t walk_pgdir(pde_ptr_t pgdir, size_t va, bool create);

// map/nmap or free memory ranges
	error_code map_range(pde_ptr_t pgdir,uintptr_t va_start,uintptr_t pa_start,size_t len);

	void free_range(pde_ptr_t pgdir, uintptr_t start, uintptr_t end);

	void unmap_range(pde_ptr_t pgdir, uintptr_t start, uintptr_t end);

	void copy_range(pde_ptr_t from, pde_ptr_t to, uintptr_t start, uintptr_t end);

} // namespace vmm


