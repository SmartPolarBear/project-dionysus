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








#include "vmm.h"

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

using namespace memory::kmem;

kmem_cache *pgdir_cache = nullptr;

void pgdir_cache_init()
{
    pgdir_cache = kmem_cache_create("pgdir", 4_KB, nullptr, nullptr, KMEM_CACHE_4KALIGN);
}

vmm::pde_ptr_t pgdir_entry_alloc()
{
    return (vmm::pde_ptr_t)kmem_cache_alloc(pgdir_cache);
}

void pgdir_entry_free(vmm::pde_ptr_t entry)
{
    kmem_cache_free(pgdir_cache, entry);
}