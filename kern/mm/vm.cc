#include "sys/vm.h"
#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"
#include "lib/libc/string.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/types.h"

#include "drivers/console/console.h"

using vm::pde_t;
using vm::pte_t;

template <typename T>
static inline auto max(T a, T b) -> T
{
    return a > b ? a : b;
}

uintptr_t max_available_phyaddr = 0ul;

static pte_t *walkpml4t(pde_t *pml4t, const void *va, bool alloc)
{

    pde_t *pml4e = &pml4t[PML4_BASE((uintptr_t)va)];
    pde_t *pdpe = nullptr;
    pde_t *pde = nullptr;
    pte_t *pte = nullptr;

    if (*pml4e & PG_P)
    {
        pdpe = (pde_t *)P2V(PTE_ADDR(*pml4e));
    }
    else
    {
        if (!alloc || (pdpe = (pde_t *)vm::bootmm_alloc()) == nullptr)
            return nullptr;

        memset(pdpe, 0, PAGE_SIZE);

        *pml4e = V2P((uintptr_t)pdpe) | PG_P | PG_W | PG_U;
    }

    if (*pdpe & PG_P)
    {
        pde = (pde_t *)P2V(PTE_ADDR(*pdpe));
    }
    else
    {
        if (!alloc || (pde = (pde_t *)vm::bootmm_alloc()) == nullptr)
            return nullptr;

        memset(pde, 0, PAGE_SIZE);

        *pdpe = V2P((uintptr_t)pde) | PG_P | PG_W | PG_U;
    }

    if (*pde & PG_P)
    {
        pte = (pte_t *)P2V(PTE_ADDR(*pde));
    }
    else
    {
        if (!alloc || (pte = (pte_t *)vm::bootmm_alloc()) == nullptr)
            return nullptr;

        memset(pte, 0, PAGE_SIZE);

        //Allow all permissions here. Modified later
        *pde = V2P((uintptr_t)pte) | PG_P | PG_W | PG_U;
    }

    return &pte[PTABLE_BASE((uintptr_t)va)];
}

static int map_pages(pde_t *kpml4t, void *va, size_t size, uintptr_t pa, size_t flags)
{
}

void vm::kvm_switch(void)
{
}

void vm::kvm_setup(size_t entrycnt, multiboot_mmap_entry entries[])
{
    //Find the limitation of physical memory
    for (size_t i = 0ul; i < entrycnt; i++)
    {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            max_available_phyaddr = max(max_available_phyaddr,
                                        PAGE_ROUNDDOWN(entries[i].addr + entries[i].len - PAGE_SIZE));
        }
    }
    // kvm_switch();
}