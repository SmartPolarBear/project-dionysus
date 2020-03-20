/*
 * Last Modified: Fri Mar 20 2020
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
#include "sys/vmm.h"

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
// TODO: lacking records of current proc
__thread cpu_info *cpu;


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

void vmm::install_gdt(void)
{
    uint8_t *local_storage = reinterpret_cast<decltype(local_storage)>(boot_alloc_page());
    memset(local_storage, 0, PAGE_SIZE);

    uint64_t *gdt = reinterpret_cast<decltype(gdt)>(local_storage);
    uint32_t *tss = reinterpret_cast<decltype(tss)>(local_storage + 1024);
    tss[16] = 0x00680000; // IO map base = 0x68

    wrmsr(MSR_FS_BASE, ((uintptr_t)local_storage) + ((PAGE_SIZE) / 2));

    gdt[0] = 0x0000000000000000;
    gdt[SEG_KCODE] = 0x0020980000000000; // Code, DPL=0, R/X
    gdt[SEG_UCODE] = 0x0020F80000000000; // Code, DPL=3, R/X
    gdt[SEG_KDATA] = 0x0000920000000000; // Data, DPL=0, W
    gdt[SEG_KCPU] = 0x0000000000000000;  // unused
    gdt[SEG_UDATA] = 0x0000F20000000000; // Data, DPL=3, W

    uintptr_t tss_addr = (uintptr_t)tss;
    gdt[SEG_TSS + 0] = (0x0067) | ((tss_addr & 0xFFFFFF) << 16) | (0x00E9LL << 40) | (((tss_addr >> 24) & 0xFF) << 56);
    gdt[SEG_TSS + 1] = (tss_addr >> 32);

    lgdt(uintptr_t(gdt), sizeof(uint64_t[8]));

    ltr(SEG_TSS << 3);

    auto c = &cpus[local_apic::get_cpunum()];
    c->local = local_storage;

    // --target=x86_64-pc-none-elf and -mcmodel=large can cause a triple fault here
    // work it around by building with x86_64-pc-linux-elf
    cpu = c;
}
