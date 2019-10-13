/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-13 22:46:26
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-13 22:48:53
 * @ Description:
 */

#include "sys/vm.h"
#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"
#include "lib/libc/string.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/types.h"

#include "drivers/console/console.h"

using vm::pde_ptr_t;
using vm::pde_t;

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

static pde_ptr_t kpml4t, kpdpt,
    iopgdir, kpgdir0,
    kpgdir1;

void vm::kvm_switch(pde_t *kpml4t)
{
    lcr3((uintptr_t)V2P<void>(kpml4t));
    console::printf("Switched to the new pml4t.\n");
}

pde_t *vm::kvm_setup(void)
{
}

void vm::kvm_init(void)
{
    kpml4t = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpdpt = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpgdir0 = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpgdir1 = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    iopgdir = reinterpret_cast<pde_ptr_t>(bootmm_alloc());

    memset(kpml4t, 0, PAGE_SIZE);
    memset(kpdpt, 0, PAGE_SIZE);
    memset(iopgdir, 0, PAGE_SIZE);

    kpml4t[511] = V2P((uintptr_t)kpdpt) | PG_P | PG_W;
    kpdpt[511] = V2P((uintptr_t)kpgdir1) | PG_P | PG_W;
    kpdpt[510] = V2P((uintptr_t)kpgdir0) | PG_P | PG_W;
    kpdpt[509] = V2P((uintptr_t)iopgdir) | PG_P | PG_W;

    for (size_t n = 0; n < PDENTRIES_COUNT; n++)
    {
        kpgdir0[n] = (n << PD_SHIFT) | PG_PS | PG_P | PG_W;
        kpgdir1[n] = ((n + 512) << PD_SHIFT) | PG_PS | PG_P | PG_W;
    }

    for (size_t n = 0; n < 16; n++)
    {
        iopgdir[n] = (DEVICE_PHYSICALBASE + (n << PD_SHIFT)) | PG_PS | PG_P | PG_W | PG_PWT | PG_PCD;
    }

    kvm_switch(kpml4t);
}

void vm::kvm_freevm(pde_t *pgdir)
{
}