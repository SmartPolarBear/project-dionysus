/*
 * Last Modified: Fri Feb 07 2020
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

#include "sys/vmm.h"
#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/memory.h"
#include "sys/mmu.h"
#include "sys/pmm.h"

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

// cpu-individual variable containing info about current CPU
__thread cpu_info *cpu;
// TODO: lacking records of current proc

// global variable for the sake of access and dynamically mapping
static pde_ptr_t g_kpml4t;

// The design of the new vm manager:
// 1) When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// 2) provide an interface to dynamically map memory
// 3) handle the page fault so as to map pages on demand

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

static inline void check_vma_overlap(struct vma_struct *prev, struct vma_struct *next)
{
    KDEBUG_ASSERT(prev->vm_start < prev->vm_end);
    KDEBUG_ASSERT(prev->vm_end <= next->vm_start);
    KDEBUG_ASSERT(next->vm_start < next->vm_end);
}

static hresult handle_pgfault([[maybe_unused]] trap_info info)
{
    uintptr_t addr = rcr2();
    //TODO: handle the page fault

    KDEBUG_RICHPANIC("Invalid access to some memory or stack overflow.",
                     "KERNEL PANIC: PAGE FAULT",
                     false,
                     "Address: 0x%p\n", addr);
    return HRES_SUCCESS;
}

void vmm::init_vmm(void)
{
    // create the global pml4t
    g_kpml4t = reinterpret_cast<pde_ptr_t>(boot_alloc_page());
    memset(g_kpml4t, 0, PHYSICAL_PAGE_SIZE);

    // register the page fault handle
    trap::trap_handle_regsiter(trap::TRAP_PGFLT, trap::trap_handle{
                                                     .handle = handle_pgfault});
}

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
void vmm::boot_map_kernel_mem(void)
{

    for (uintptr_t pa = 0, va = KERNEL_VIRTUALBASE;
         pa <= KERNEL_VIRTUALEND;
         pa += PHYSICAL_PAGE_SIZE, va += PHYSICAL_PAGE_SIZE)
    {
        auto ret = map_page(g_kpml4t, va, pa, PG_W);

        if (ret == ERROR_SUCCESS)
        {
            KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
        }
        else if (ret == ERROR_REMAP)
        {
            KDEBUG_RICHPANIC("Remap a mapped page.", "KERNEL PANIC:ERROR_REMAP",
                             true, " tring to map 0x%x to 0x%x", pa, va);
        }
    }
}

void vmm::install_gdt(void)
{
    uint32_t *tss = nullptr;
    uint64_t *gdt = nullptr;
    uint8_t *local_storage = reinterpret_cast<decltype(local_storage)>(boot_alloc_page());

    memset(local_storage, 0, PHYSICAL_PAGE_SIZE);

    gdt = reinterpret_cast<decltype(gdt)>(local_storage);

    tss = reinterpret_cast<decltype(tss)>(local_storage + 1024);
    tss[16] = 0x00680000;

    wrmsr(0xC0000100, ((uintptr_t)local_storage) + ((PHYSICAL_PAGE_SIZE) / 2));

    auto c = &cpus[local_apic::get_cpunum()];
    c->local = local_storage;

    cpu = c;
    // TODO: lacking initialization of proc

    auto tss_addr = (uintptr_t)tss;
    gdt[0] = 0x0000000000000000;
    gdt[SEG_KCODE] = 0x0020980000000000; // Code, DPL=0, R/X
    gdt[SEG_UCODE] = 0x0020F80000000000; // Code, DPL=3, R/X
    gdt[SEG_KDATA] = 0x0000920000000000; // Data, DPL=0, W
    gdt[SEG_KCPU] = 0x0000000000000000;  // unused
    gdt[SEG_UDATA] = 0x0000F20000000000; // Data, DPL=3, W
    gdt[SEG_TSS + 0] = (0x0067) | ((tss_addr & 0xFFFFFF) << 16) |
                       (0x00E9LL << 40) | (((tss_addr >> 24) & 0xFF) << 56);
    gdt[SEG_TSS + 1] = (tss_addr >> 32);

    lgdt(reinterpret_cast<gdt_segment *>(gdt), sizeof(uint64_t[8]));

    ltr(SEG_TSS << 3);
}

vma_struct *vmm::find_vma(mm_struct *mm, uintptr_t addr)
{
    KDEBUG_ASSERT(mm != nullptr);
    vma_struct *ret = mm->mmap_cache;
    if (!(ret != nullptr &&
          ret->vm_start <= addr &&
          ret->vm_end > addr))

    {
        ret = nullptr;
        list_head *entry = nullptr;
        list_for(entry, &mm->mmap_list)
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

vma_struct *vmm::vma_create(uintptr_t vm_start, uintptr_t vm_end, size_t vm_flags)
{
    vma_struct *vma = reinterpret_cast<decltype(vma)>(memory::kmalloc(sizeof(vma_struct)));

    if (vma != nullptr)
    {
        vma->vm_start = vm_start;
        vma->vm_end = vm_end;
        vma->flags = vm_flags;
    }

    return vma;
}

void vmm::insert_vma_struct(mm_struct *mm, vma_struct *vma)
{
    KDEBUG_ASSERT(vma->vm_start < vma->vm_end);

    // find the place to insert to
    list_head *prev = nullptr;

    list_head *iter = nullptr;
    list_for(iter, &mm->mmap_list)
    {
        auto prev_vma = container_of(iter, vma_struct, vma_link);
        if (prev_vma->vm_start > vma->vm_start)
        {
            break;
        }
        prev = iter;
    }

    auto next = prev->next;

    if (prev != &mm->mmap_list)
    {
        check_vma_overlap(container_of(prev, vma_struct, vma_link), vma);
    }

    if (next != &mm->mmap_list)
    {
        check_vma_overlap(vma, container_of(next, vma_struct, vma_link));
    }

    vma->mm = mm;
    list_add(&vma->vma_link, prev);

    mm->map_count++;
}

mm_struct *vmm::mm_create(void)
{
    mm_struct *mm = reinterpret_cast<decltype(mm)>(memory::kmalloc(sizeof(mm_struct)));
    if (mm != nullptr)
    {
        list_init(&mm->mmap_list);

        mm->mmap_cache = nullptr;
        mm->pgdir = nullptr;
        mm->map_count = 0;
    }

    return mm;
}

void vmm::mm_destroy(mm_struct *mm)
{
    list_head *iter = nullptr;
    list_for(iter, &mm->mmap_list)
    {
        list_remove(iter);
        memory::kfree(container_of(iter, vma_struct, vma_link));
    }
    memory::kfree(mm);
    mm = nullptr;
}
