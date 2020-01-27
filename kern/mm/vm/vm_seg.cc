/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-15 20:03:19
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-12-12 23:11:54
 * @ Description:
 */

#include "sys/allocators/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/types.h"
#include "sys/vm.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "arch/amd64/x86.h"

#include "lib/libc/string.h"

 __thread cpu_info *cpu;
// TODO: lacking records of proc

void vm::segment::init_segmentation(void)
{
    uint32_t *tss = nullptr;
    uint64_t *gdt = nullptr;
    auto local_storage = vm::bootmm_alloc();

    memset(local_storage, 0, vm::BOOTMM_BLOCKSIZE);

    gdt = reinterpret_cast<decltype(gdt)>(local_storage);

    tss = reinterpret_cast<decltype(tss)>(local_storage + 1024);
    tss[16] = 0x00680000;

    wrmsr(0xC0000100, ((uintptr_t)local_storage) + ((vm::BOOTMM_BLOCKSIZE) / 2));

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