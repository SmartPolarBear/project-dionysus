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


// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

vmm::pde_ptr_t pgdir_entry_alloc();

void pgdir_entry_free(vmm::pde_ptr_t entry);

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


