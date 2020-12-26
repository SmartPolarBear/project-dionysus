/*
 * Last Modified: Sat May 16 2020
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

#include "system/memlayout.h"
#include "system/types.h"
#include "system/vmm.h"

#include "debug/kdebug.h"
#include "drivers/lock/spinlock.h"
#include "drivers/apic/traps.h"


using vmm::pde_ptr_t;
namespace pmm
{

    constexpr size_t PMM_MANAGER_NAME_MAXLEN = 32;

    struct pmm_desc
    {
        char name[PMM_MANAGER_NAME_MAXLEN];

        void (*init)(void);                             // initialize internal description&management kbl structure
        // (free superblock list, number of free superblock) of XXX_pmm_manager
        void (*init_memmap)(page_info *base, size_t n); // setup description&management kbl structcure according to
        // the initial free physical memory space
        page_info *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm
        void (*free_pages)(page_info *base,
                           size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
        size_t (*get_free_pages_count)(void);           // return the number of free pages

        lock::spinlock lock;
        bool enable_lock;
    };

    extern pmm_desc *pmm_entity;

    extern page_info *pages;
    extern size_t page_count;

    void init_pmm(void);

    page_info *alloc_pages(size_t n);

    void free_pages(page_info *base, size_t n);

    size_t get_free_page_count(void);

    void page_remove(pde_ptr_t pgdir, uintptr_t va_to_page);

    void tlb_invalidate(pde_ptr_t pgdir, uintptr_t va);

    error_code page_insert(pde_ptr_t pgdir, bool allow_rewrite, page_info *page, uintptr_t va, size_t perm);

	// insert a free page to pgdir
    error_code
    pgdir_alloc_page(IN pde_ptr_t pgdir, bool rewrite_if_exist, uintptr_t va, size_t perm, OUT page_info **page);

    error_code pgdir_alloc_pages(IN pde_ptr_t pgdir, bool rewrite_if_exist, size_t n, uintptr_t va, size_t perm, OUT
                                 page_info **page);

    static inline uintptr_t pavailable_start(void)
    {
        return V2P(roundup((uintptr_t) (&pages[page_count]), PAGE_SIZE));
    }

    static inline page_info *alloc_page(void)
    {
        return alloc_pages(1);
    }

    static inline void free_page(page_info *pg)
    {
        free_pages(pg, 1);
    }

    static inline size_t page_to_index(page_info *pg)
    {
        return pg - pages;
    }

    static inline uintptr_t page_to_pa(page_info *pg)
    {
        auto ret = pavailable_start() + page_to_index(pg) * PAGE_SIZE;
        return ret;
    }

    static inline uintptr_t page_to_va(page_info *pg)
    {
        KDEBUG_ASSERT(pg != nullptr);
        return P2V(page_to_pa(pg));
    }

    static inline page_info *pa_to_page(uintptr_t pa)
    {
        size_t index = rounddown((pa - (V2P((uintptr_t) &pages[page_count]))), PAGE_SIZE) / PAGE_SIZE;
        KDEBUG_ASSERT(index < page_count);
        return &pages[index];
    }

    static inline page_info *va_to_page(uintptr_t va)
    {
        return pa_to_page(V2P(va));
    }

    static inline page_info *pde_to_page(pde_ptr_t pde)
    {
        return pa_to_page(vmm::pde_to_pa(pde));
    }

    namespace boot_mem
    {
        static inline void *boot_alloc_page(void)
        {
            // shouldn't have interrupts.
            // popcli&pushcli can neither be used because unprepared cpu local storage
            KDEBUG_ASSERT(!(read_eflags() & trap::EFLAG_IF));

            // we don't reuse alloc_page() because spinlock may not be prepared.
            auto page = pmm_entity->alloc_pages(1);
            KDEBUG_ASSERT(page != nullptr);

            return reinterpret_cast<void *>(page_to_va(page));
        }
    } // namespace boot_mem

} // namespace pmm
