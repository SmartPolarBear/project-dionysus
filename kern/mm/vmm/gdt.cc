/*
 * Last Modified: Mon May 04 2020
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

#include "sys/error.h"
#include "sys/kmalloc.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/pmm.h"
#include "sys/segmentation.hpp"
#include "sys/vmm.h"

#include "arch/amd64/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include <cstring>
#include <algorithm>

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
// TODO: lacking records of current proc
__thread cpu_struct *cpu;

void vmm::install_gdt(void)
{
    auto current_cpu = &cpus[local_apic::get_cpunum()];

    uint8_t *cpu_fs = reinterpret_cast<decltype(cpu_fs)>(boot_alloc_page());
    memset(cpu_fs, 0, PAGE_SIZE);
    wrmsr(MSR_FS_BASE, ((uintptr_t)cpu_fs));
    current_cpu->local_fs = cpu_fs;

    uint8_t *cpu_kernel_gs = reinterpret_cast<decltype(cpu_kernel_gs)>(boot_alloc_page());
    memset(cpu_kernel_gs, 0, PAGE_SIZE);
    wrmsr(MSR_KERNEL_GS_BASE, ((uintptr_t)cpu_kernel_gs));
    current_cpu->local_gs = cpu_kernel_gs;

    uintptr_t tss_addr = (uintptr_t)(&current_cpu->tss);

    current_cpu->gdt_table.tss_low.base15_0 = tss_addr & 0xffff;
    current_cpu->gdt_table.tss_low.base23_16 = (tss_addr >> 16) & 0xff;
    current_cpu->gdt_table.tss_low.base31_24 = (tss_addr >> 24) & 0xff;
    current_cpu->gdt_table.tss_low.limit15_0 = sizeof(task_state_segment);
    current_cpu->gdt_table.tss_high.limit15_0 = (tss_addr >> 32) & 0xffff;

    current_cpu->install_gdt_and_tss();

    // --target=x86_64-pc-none-elf and -mcmodel=large can cause a triple fault here
    // work it around by building with x86_64-pc-linux-elf
    cpu = current_cpu;
}

void vmm::tss_set_rsp(uint32_t *tss, size_t n, uint64_t rsp)
{
    tss[n * 2 + 1] = rsp;
    tss[n * 2 + 2] = rsp >> 32;
}

uint64_t vmm::tss_get_rsp(uint32_t *tss, size_t n)
{
    uint64_t ret = (((uint64_t)tss[n * 2 + 2]) << 32ull) | ((uint64_t)tss[n * 2 + 1]);
    return ret;
}
