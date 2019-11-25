/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-15 20:03:19
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-24 17:52:27
 * @ Description:
 */

#include "arch/amd64/x86.h"
#include "lib/libc/string.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/types.h"
#include "sys/vm.h"

#include "drivers/acpi/cpu.h"

__thread struct cpu *cpu;

static inline void make_gate(uint32_t *idt, uint32_t n,
                             void *kva,
                             DescriptorPrivilegeLevel pl,
                             ExceptionType trap)
{
    uintptr_t addr = (uintptr_t)kva;
    n *= 4;
    idt[n + 0] = (addr & 0xFFFF) | ((SEG_KCODE << 3) << 16);
    idt[n + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
    idt[n + 2] = addr >> 32;
    idt[n + 3] = 0;
}

void vm::segment::init_segment(void)
{
}