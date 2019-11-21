/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-13 22:46:26
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-21 23:46:51
 * @ Description:
 */

#include "sys/vm.h"

#include "boot/multiboot2.h"

#include "sys/bootmm.h"
#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"
#include "sys/types.h"

#include "arch/amd64/x86.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"

using vm::pde_ptr_t;
using vm::pde_t;

using vm::bootmm_alloc;

// global kmpl4t ptr for convenience
static pde_ptr_t g_kpml4t;

static inline void map_kernel_text(const pde_ptr_t kpml4t)
{
    using vm::bootmm_alloc;

    pde_ptr_t kpdpt = reinterpret_cast<pde_ptr_t>(bootmm_alloc()),
              kpgdir0 = reinterpret_cast<pde_ptr_t>(bootmm_alloc()),
              kpgdir1 = reinterpret_cast<pde_ptr_t>(bootmm_alloc()),
              iopgdir = reinterpret_cast<pde_ptr_t>(bootmm_alloc());

    KDEBUG_ASSERT(kpdpt != nullptr && kpgdir0 != nullptr && kpgdir1 != nullptr && iopgdir != nullptr);

    memset(kpdpt, 0, PAGE_SIZE);
    memset(kpgdir0, 0, PAGE_SIZE);
    memset(kpgdir1, 0, PAGE_SIZE);
    memset(iopgdir, 0, PAGE_SIZE);

    // Map [0,2GB) to -2GB from the top virtual address.

    kpml4t[511] = V2P((uintptr_t)kpdpt) | PG_P | PG_W;

    // kpgdir1 is for [0,-1GB)
    kpdpt[511] = V2P((uintptr_t)kpgdir1) | PG_P | PG_W;
    // kpgdir0 is for [-1GB,-2GB)
    kpdpt[510] = V2P((uintptr_t)kpgdir0) | PG_P | PG_W;

    kpdpt[509] = V2P((uintptr_t)iopgdir) | PG_P | PG_W;

    for (size_t n = 0; n < PDENTRIES_COUNT; n++)
    {
        kpgdir0[n] = (n << P2_SHIFT) | PG_PS | PG_P | PG_W;
        kpgdir1[n] = ((n + 512) << P2_SHIFT) | PG_PS | PG_P | PG_W;
    }

    for (size_t n = 0; n < 16; n++)
    {
        iopgdir[n] = (DEVICE_PHYSICALBASE + (n << P2_SHIFT)) | PG_PS | PG_P | PG_W | PG_PWT | PG_PCD;
    }
}

static inline RESULT map_addr(const pde_ptr_t kpml4t, uintptr_t vaddr, uintptr_t paddr)
{
    auto remove_flags = [](size_t pde) {
        constexpr size_t FLAGS_SHIFT = 8;
        return (pde >> FLAGS_SHIFT) << FLAGS_SHIFT;
    };

    // the kpml4, which's the content of CR3, is held in kpml4t
    // find the 3rd page directory (PDPT) from it.
    pde_ptr_t pml4e = &kpml4t[P4X(vaddr)],
              pdpt = nullptr,
              pdpte = nullptr,
              pgdir = nullptr,
              pde = nullptr;

    if (!(*pml4e & PG_P))
    {
        pdpt = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
        KDEBUG_ASSERT(pdpt != nullptr);
        if (pdpt == nullptr)
        {
            return ERROR_MEMORY_ALLOC;
        }
        memset(pdpt, 0, PAGE_SIZE);

        *pml4e = ((V2P((uintptr_t)pdpt)) | PG_P | PG_W);
    }
    else
    {
        pdpt = reinterpret_cast<decltype(pdpt)>(P2V(remove_flags(*pml4e)));
    }

    // find the 2nd page directory from PGPT
    pdpte = &pdpt[P3X(vaddr)];
    if (!(*pdpte & PG_P))
    {
        pgdir = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
        KDEBUG_ASSERT(pgdir != nullptr);
        if (pgdir == nullptr)
        {
            return ERROR_MEMORY_ALLOC;
        }
        memset(pgdir, 0, PAGE_SIZE);

        *pdpte = ((V2P((uintptr_t)pgdir)) | PG_P | PG_W);
    }
    else
    {
        pgdir = reinterpret_cast<decltype(pgdir)>(P2V(remove_flags(*pdpte)));
    }

    // find the page from 2nd page directory.
    // because we use page size extension (2mb pages)
    // this is the last level.
    pde = &pgdir[P2X(vaddr)];
    if (!(*pde & PG_P))
    {
        *pde = ((paddr) | PG_PS | PG_P | PG_W);
    }

    return ERROR_SUCCESS;
}

static inline void map_whole_physical(const pde_ptr_t kpml4t)
{
    // get mmap tag from multiboot info and find the limit of physical memory
    auto memtag = (multiboot_tag_mmap *)multiboot::aquire_tag(MULTIBOOT_TAG_TYPE_MMAP);
    size_t entry_count = (memtag->size - sizeof(multiboot_uint32_t) * 4ul - sizeof(memtag->entry_size)) / memtag->entry_size;

    uintptr_t end_addr = 0;
    auto max_size = 0ull;

    for (size_t i = 0; i < entry_count; i++)
    {
        auto entry = memtag->entries + i;
        max_size = sysstd::max(max_size, sysstd::min(entry->addr + entry->len, (unsigned long long)PHYMEMORY_SIZE));
    }

    max_size = PGROUNDDOWN(max_size);
    KDEBUG_ASSERT(max_size > 0 && max_size < PHYMEMORY_SIZE);

    // the physical memory address is between [0,maxsize]

    for (uintptr_t addr = 0; (addr + 2_MB) <= max_size; addr += 2_MB)
    {
        uintptr_t virtual_addr = addr + PHYREMAP_VIRTUALBASE;
        KDEBUG_ASSERT(virtual_addr < PHYREMAP_VIRTUALEND);
        if (map_addr(kpml4t, virtual_addr, addr) != ERROR_SUCCESS)
        {
            KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
        }
    }
}

void vm::switch_kernelvm()
{
    lcr3(V2P((uintptr_t)g_kpml4t));
}

pde_t *vm::setup_kernelvm(void)
{
    KDEBUG_NOT_IMPLEMENTED;
}

void vm::init_kernelvm(void)
{
    // allocate pml4t and initialize it with 0
    g_kpml4t = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    memset(g_kpml4t, 0, PAGE_SIZE);

    // map kernel from address 0 to KERNEL_VIRTUALBASE
    map_kernel_text(g_kpml4t);

    // map whole physical memory again to PHYREMAP_VIRTUALBASE
    map_whole_physical(g_kpml4t);

    // install the PML4T to CR3
    switch_kernelvm();
}

static size_t next_iopgdir_idx = 511;
uintptr_t vm::map_io_addr(uintptr_t addrst, size_t sz)
{

    switch_kernelvm();
}

void vm::freevm(pde_t *pgdir)
{
}