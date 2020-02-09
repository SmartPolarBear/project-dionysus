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

#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/memory.h"
#include "sys/mmu.h"
#include "sys/pmm.h"
#include "sys/vmm.h"

#include "arch/amd64/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"

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

// global variable for the sake of access and dynamically mapping
pde_ptr_t g_kpml4t;

// find the pde corresponding to the given va
// perm is only valid when create_if_not_exist = true
static inline pde_ptr_t walk_pgdir(const pde_ptr_t pml4t,
                                   uintptr_t vaddr,
                                   bool create_if_not_exist = false,
                                   size_t perm = 0)
{
    // the lambda removes all flags in entries to expose the address of the next level
    auto remove_flags = [](size_t pde) {
        constexpr size_t FLAGS_SHIFT = 8;
        return (pde >> FLAGS_SHIFT) << FLAGS_SHIFT;
    };

    // the pml4, which's the content of CR3, is held in kpml4t
    // firstly find the 3rd page directory (PDPT) from it.
    pde_ptr_t pml4e = &pml4t[P4X(vaddr)],
              pdpt = nullptr,
              pdpte = nullptr,
              pgdir = nullptr,
              pde = nullptr;

    if (!(*pml4e & PG_P))
    {
        if (!create_if_not_exist)
        {
            return nullptr;
        }

        pdpt = reinterpret_cast<pde_ptr_t>(boot_alloc_page());
        KDEBUG_ASSERT(pdpt != nullptr);
        if (pdpt == nullptr)
        {
            return nullptr;
        }

        memset(pdpt, 0, PHYSICAL_PAGE_SIZE);
        *pml4e = ((V2P((uintptr_t)pdpt)) | PG_P | perm);
    }
    else
    {
        pdpt = reinterpret_cast<decltype(pdpt)>(P2V(remove_flags(*pml4e)));
    }

    // find the 2nd page directory from PGPT
    pdpte = &pdpt[P3X(vaddr)];
    if (!(*pdpte & PG_P))
    {
        if (!create_if_not_exist)
        {
            return nullptr;
        }

        pgdir = reinterpret_cast<pde_ptr_t>(boot_alloc_page());
        KDEBUG_ASSERT(pgdir != nullptr);
        if (pgdir == nullptr)
        {
            return nullptr;
        }

        memset(pgdir, 0, PHYSICAL_PAGE_SIZE);
        *pdpte = ((V2P((uintptr_t)pgdir)) | PG_P | perm);
    }
    else
    {
        pgdir = reinterpret_cast<decltype(pgdir)>(P2V(remove_flags(*pdpte)));
    }

    // find the page from 2nd page directory.
    // because we use page size extension (2mb pages)
    // this is the last level.
    pde = &pgdir[P2X(vaddr)];

    return pde;
}

// this method maps the specific va
static inline RESULT map_page(pde_ptr_t pml4, uintptr_t va, uintptr_t pa, size_t perm)
{
    auto pde = walk_pgdir(pml4, va, true, perm);

    if (pde == nullptr)
    {
        return ERROR_MEMORY_ALLOC;
    }

    if (!(*pde & PG_P))
    {
        *pde = ((pa) | PG_PS | PG_P | perm);
    }
    else
    {
        return ERROR_REMAP;
    }

    return ERROR_SUCCESS;
}

static inline hresult map_pages(pde_ptr_t pml4, uintptr_t va_start, uintptr_t va_end, uintptr_t pa_start)
{
    hresult ret;
    // map the kernel memory
    for (uintptr_t pa = pa_start, va = va_start;
         va <= va_end;
         pa += PHYSICAL_PAGE_SIZE, va += PHYSICAL_PAGE_SIZE)
    {
        ret = map_page(pml4, va, pa, PG_W);

        if (ret != ERROR_SUCCESS)
        {
            return ret;
        }
    }

    return ret;
}

void vmm::install_kpml4()
{
    lcr3(V2P((uintptr_t)g_kpml4t));
}

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// and then map all the memories to PHYREMAP_VIRTUALBASE
void vmm::boot_map_kernel_mem(uintptr_t max_pa_addr)
{
    // map the kernel memory
    auto ret = map_pages(g_kpml4t, KERNEL_VIRTUALBASE, KERNEL_VIRTUALEND, 0);

    if (ret == ERROR_MEMORY_ALLOC)
    {
        KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
    }
    else if (ret == ERROR_REMAP)
    {
        KDEBUG_RICHPANIC("Remap a mapped page.", "KERNEL PANIC:ERROR_REMAP",
                         true, "");
    }

    // remap all the physical memory
    ret = map_pages(g_kpml4t, PHYREMAP_VIRTUALBASE, max_pa_addr, 0);

    if (ret == ERROR_MEMORY_ALLOC)
    {
        KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
    }
    else if (ret == ERROR_REMAP)
    {
        KDEBUG_RICHPANIC("Remap a mapped page.", "KERNEL PANIC:ERROR_REMAP",
                         true, "");
    }
    install_kpml4();
}

uintptr_t vmm::pde_to_pa(pde_ptr_t pde)
{
    constexpr size_t FLAGS_SHIFT = 8;
    return ((((*pde) >> FLAGS_SHIFT) << FLAGS_SHIFT) & (~PG_PS));
}

pde_ptr_t vmm::walk_pgdir(pde_ptr_t pgdir, size_t va, bool create)
{
    return ::walk_pgdir(pgdir, va, create, PG_U);
}
