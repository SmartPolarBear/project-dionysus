/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-13 22:46:26
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-27 23:26:52
 * @ Description:
 */

#include "sys/vm.h"
#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"
#include "lib/libc/string.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"
#include "sys/types.h"

#include "drivers/console/console.h"

using vm::pde_ptr_t;
using vm::pde_t;

static pde_ptr_t kpml4t, kpdpt,
    iopgdir, kpgdir0,
    kpgdir1;

void vm::switch_kernelvm()
{
    lcr3(V2P((uintptr_t)kpml4t));
}

pde_t *vm::setup_kernelvm(void)
{
    return nullptr;
}

void vm::init_kernelvm(void)
{
    kpml4t = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpdpt = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpgdir0 = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    kpgdir1 = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    iopgdir = reinterpret_cast<pde_ptr_t>(bootmm_alloc());

    memset(kpml4t, 0, PAGE_SIZE);
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
        kpgdir0[n] = (n << PDX_SHIFT) | PG_PS | PG_P | PG_W;
        kpgdir1[n] = ((n + 512) << PDX_SHIFT) | PG_PS | PG_P | PG_W;
    }

    for (size_t n = 0; n < 16; n++)
    {
        iopgdir[n] = (DEVICE_PHYSICALBASE + (n << PDX_SHIFT)) | PG_PS | PG_P | PG_W | PG_PWT | PG_PCD;
    }

    switch_kernelvm();
}

void vm::freevm(pde_t *pgdir)
{
}