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

constexpr size_t MAX_KMAP_ENTRY_COUNT = 4;

struct kmap_entry
{
    void *vaddr;
    uintptr_t paddr_start;
    uintptr_t paddr_end;
    size_t flags;
    bool use;
};

struct kmap
{
    uintptr_t max_available_phyaddr;
    uintptr_t min_available_phyaddr;
    kmap_entry entries[MAX_KMAP_ENTRY_COUNT];

    kmap() : max_available_phyaddr(0), min_available_phyaddr(UINTPTR_MAX)
    {
        for (auto e : entries)
        {
            e.use = false;
        }
    }
} kmap;

extern char data[];  //kernel.ld
extern char edata[]; //kernel.ld

pde_t *kernel_pml4t = nullptr;

template <typename T>
static inline auto max(T a, T b) -> T
{
    return a < b ? b : a;
}

template <typename T>
static inline auto min(T a, T b) -> T
{
    return a < b ? a : b;
}

static pte_t *walk_pml4t(pde_t *pml4t, const void *va, bool alloc)
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
    char *addr = nullptr, *last = nullptr;
    pde_t *pte = nullptr;

    addr = (char *)PAGE_ROUNDDOWN((uintptr_t)va);
    last = (char *)PAGE_ROUNDDOWN(((uintptr_t)va) + size - 1);

    for (;;)
    {
        if ((pte = walk_pml4t(kpml4t, addr, true)) == nullptr)
        {
            return -1;
        }

        // if (*pte & PG_P)
        // {
        //     //TODO: panic the kernel for remapping
        //     console::printf("remap!!\n");
        //     console::printf(":%d\n", (int)*pte);
        //     return -2;
        // }

        *pte = pa | flags | PG_P;

        if (addr == last)
        {
            break;
        }

        addr += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    return 0;
}

static void fill_kmap(void)
{

    kmap.entries[0] = (kmap_entry){
        (void *)KERNEL_VIRTUALBASE,
        0,
        kmap.min_available_phyaddr,
        PG_W | PG_U | PG_P,
        true};

    kmap.entries[1] = (kmap_entry){
        (void *)(KERNEL_VIRTUALBASE + kmap.min_available_phyaddr),
        kmap.min_available_phyaddr,
        V2P((uintptr_t)data),
        PG_W | PG_U | PG_P,
        true};

    kmap.entries[2] = (kmap_entry){
        0,
        0,
        kmap.min_available_phyaddr,
        PG_W | PG_U | PG_P,
        false};

    kmap.entries[3] = (kmap_entry){
        (void *)P2V(kmap.max_available_phyaddr - DEVSPACE_SIZE),
        kmap.max_available_phyaddr - DEVSPACE_SIZE,
        kmap.max_available_phyaddr,
        PG_W | PG_U | PG_P,
        false}; //Attention
}

void vm::kvm_switch(pde_t *kpml4t)
{
    lcr3((uintptr_t)V2P<void>(kpml4t));

    // console::printf("kvm_switch\n");
}

pde_t *vm::kvm_setup(void)
{
    pde_t *kpml4t = (pde_t *)bootmm_alloc();
    if (!kpml4t)
    {
        return nullptr;
    }

    memset(kpml4t, 0, PAGE_SIZE);

    for (auto e : kmap.entries)
    {
        if (map_pages(kpml4t, e.vaddr,
                      e.paddr_end - e.paddr_start,
                      e.paddr_start, e.flags) < 0)
        {
            console::printf("fuck! 0x%x\n", e.paddr_start);
            kvm_freevm(kpml4t);
            return nullptr;
        }
    }
    return kpml4t;
}

void vm::kvm_init(size_t entrycnt, multiboot_mmap_entry entries[])
{
    //Find the limitation of physical memory
    for (size_t i = 0ul; i < entrycnt; i++)
    {
        console::printf("addr=0x%x (%d), len=%x (%d), type=%d\n", entries[i].addr, entries[i].addr, entries[i].len, entries[i].len, entries[i].type);
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            kmap.min_available_phyaddr = max(kmap.max_available_phyaddr,
                                             PAGE_ROUNDDOWN(entries[i].addr + PAGE_SIZE));
            kmap.max_available_phyaddr = max(kmap.max_available_phyaddr,
                                             PAGE_ROUNDDOWN(entries[i].addr + entries[i].len - PAGE_SIZE));
        }
    }

    fill_kmap();

    kernel_pml4t = kvm_setup();
    // kvm_switch(kernel_pml4t);

    // int test = 0;
    // int error = 0;
    // for (int i = KERNEL_VIRTUALBASE + 0xabcde; i < KERNEL_VIRTUALBASE + 0xedcba; i++)
    // {
    //     test = i;
    //     int *ptest = &test;

    //     pte_t *pte = walk_pml4t(kernel_pml4t, (void *)ptest, false);
    //     // console::printf("looking up 0x%x\n", (void *)ptest);
    //     // break;
    //     if (pte == nullptr)
    //     {
    //         console::printf("No mapping!\n");
    //         error++;
    //     }
    //     else
    //     {
    //         int offset = ((uintptr_t)ptest) & 0b111111111111;
    //         int page = PTABLE_BASE(*pte);
    //         int addr = (page << 12) | offset;
    //         if (addr != (uintptr_t)V2P(ptest))
    //         {
    //             console::printf("WRONG! page=0x%x off=0x%x  v2p=0x%x trans=0x%x\n", page, offset, (uintptr_t)ptest, addr);
    //             error++;
    //         }
    //         else
    //         {
    //             console::printf("right! 0x%x 0x%x | 0x%x 0x%x\n", page, offset, (uintptr_t)V2P(ptest), addr);
    //             error++;
    //         }
    //     }

    //     if (error >= 8)
    //         return;
    // }
}

void vm::kvm_freevm(pde_t *pgdir)
{
    if (pgdir == nullptr)
    {
        //TODO: panic for freeing null pgdir
        console::printf("freeing null pgdir");
        return;
    }

    //TODO: deallocate user vm

    for (size_t i = 0; i < 512; i++)
    {
        if (pgdir[i] & PG_P)
        {
            char *v = (char *)P2V(PTE_ADDR(pgdir[i]));
            bootmm_free(v);
        }
    }
    bootmm_free((char *)pgdir);
}