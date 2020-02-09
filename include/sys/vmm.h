/*
 * Last Modified: Sun Feb 09 2020
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

#if !defined(__INCLUDE_SYS_VMM_H)
#define __INCLUDE_SYS_VMM_H

#include "sys/types.h"

namespace vmm
{
using pde_t = size_t;
using pde_ptr_t = pde_t *;

struct vma_struct;

struct mm_struct
{
    // TODO: optimize with trees
    list_head mmap_list;    // linked list of vma structures
    vma_struct *mmap_cache; // for quicker search of vma
    pde_ptr_t pgdir;
    size_t map_count;
};

struct vma_struct
{
    mm_struct *mm; // the mm this struct belongs to
    uintptr_t vm_start;
    uintptr_t vm_end;
    size_t flags;
    // TODO: optimize it with trees
    list_head vma_link;
};

enum VM_FLAGS : size_t
{
    VM_READ = 0b1,
    VM_EXEC = 0b100,
    VM_WRITE = 0b10,
};

vma_struct *find_vma(mm_struct *mm, uintptr_t addr);
vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end, size_t vm_flags);
void insert_vma_struct(mm_struct *mm, vma_struct *vma);

mm_struct *mm_create(void);
void mm_destroy(mm_struct *mm);

// initialize the vmm
// 1) register the page fault handle
// 2) allocate an pml4 table
void init_vmm(void);

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// and then map all the memories to PHYREMAP_VIRTUALBASE
void boot_map_kernel_mem(uintptr_t max_pa_addr);

// install GDT
void install_gdt(void);
// install g_kml4_t to cr3
void install_kpml4(void);
// get the physical address mapped by a pde
uintptr_t pde_to_pa(pde_ptr_t pde);
pde_ptr_t walk_pgdir(pde_ptr_t pgdir, size_t va, bool create);

} // namespace vmm

#endif // __INCLUDE_SYS_VMM_H
