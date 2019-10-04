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
using vm::pdpt_t;
using vm::pgdir_t;
using vm::pml4_t;

static pml4_t *kpml4;
static pdpt_t *kpdpt;
static pgdir_t *iopgdir, *kpgdir0, *kpgdir1;

void vm::kvm_switch(void)
{
    auto pml4_addr = (uintptr_t)V2P(kpml4);
    console::printf("kpml4=%d\n", (uintptr_t)V2P(kpml4));

    lcr3((uintptr_t)V2P(kpml4));
}

void vm::kvm_setup(size_t entrycnt, multiboot_mmap_entry entries[])
{
    for (int i = 0; i < entrycnt; i++)
    {
        console::printf("add=0x%x,len=0x%x,type=%d\n", entries[i].addr, entries[i].len, entries[i].type);
    }

    kpml4 = reinterpret_cast<decltype(kpml4)>(bootmm_alloc());
    kpdpt = reinterpret_cast<decltype(kpdpt)>(bootmm_alloc());
    kpgdir0 = reinterpret_cast<decltype(kpgdir0)>(bootmm_alloc());
    kpgdir1 = reinterpret_cast<decltype(kpgdir1)>(bootmm_alloc());
    iopgdir = reinterpret_cast<decltype(iopgdir)>(bootmm_alloc());

    memset(kpml4, 0, PGSIZE);
    memset(kpdpt, 0, PGSIZE);
    memset(iopgdir, 0, PGSIZE);
    memset(kpgdir0, 0, PGSIZE);
    memset(kpgdir1, 0, PGSIZE);

    kpml4[PDT_ENTRY_COUNT - 1] = ((uintptr_t)V2P(kpdpt)) | PTE_P | PTE_W ;

    kpdpt[PDT_ENTRY_COUNT - 1] = ((uintptr_t)V2P(kpgdir1)) | PTE_P | PTE_W ;
    kpdpt[PDT_ENTRY_COUNT - 2] = ((uintptr_t)V2P(kpgdir0)) | PTE_P | PTE_W ;
    kpdpt[PDT_ENTRY_COUNT - 3] = ((uintptr_t)V2P(iopgdir)) | PTE_P | PTE_W ;

    for (size_t n = 0; n < PDT_ENTRY_COUNT; n++)
    {
        kpgdir0[n] = (n << PDXSHIFT) | PTE_PS | PTE_P | PTE_W;
        kpgdir1[n] = ((n + PDT_ENTRY_COUNT) << PDXSHIFT) | PTE_PS | PTE_P | PTE_W;
    }

    for (size_t n = 0; n < 16; n++)
    {
        iopgdir[n] = (DEVICE_PHYSICAL + (n << PDXSHIFT)) | PTE_PS | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
    }

    kvm_switch();
}