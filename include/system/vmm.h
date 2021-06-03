

// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "system/types.h"

#include "memory/fpage.hpp"

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

void vma_destroy(vma_struct* vma);

void insert_vma_struct(mm_struct* mm, vma_struct* vma);

mm_struct* mm_create(void);

error_code mm_map(IN mm_struct* mm, IN uintptr_t addr, IN size_t len, IN uint32_t vm_flags,
	OPTIONAL OUT vma_struct** vma_store);

error_code mm_fpage_map(mm_struct* from,
	mm_struct* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive,
	vma_struct** vma_store);

error_code mm_fpage_grant(mm_struct* from,
	mm_struct* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive,
	vma_struct** vma_store);

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
void init_vmm();

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// and then map all the memories to PHYREMAP_VIRTUALBASE
void paging_init();

// install GDT
void install_gdt();

// install g_kml4_t to cr3
void install_kernel_pml4t();

// get a copy of the kernel pml4t

void duplicate_kernel_pml4t(OUT pde_ptr_t pml4t);

// get the physical address mapped by a pde
uintptr_t pde_to_pa(pde_ptr_t pde);

pde_ptr_t walk_pgdir(pde_ptr_t pgdir, size_t va, bool create);

// map/nmap or free memory ranges
error_code map_range(pde_ptr_t pgdir, uintptr_t va_start, uintptr_t pa_start, size_t len);

void free_range(pde_ptr_t pgdir, uintptr_t start, uintptr_t end);

void unmap_range(pde_ptr_t pgdir, uintptr_t start, uintptr_t end);

void copy_range(pde_ptr_t from, pde_ptr_t to, uintptr_t start, uintptr_t end);

} // namespace vmm


