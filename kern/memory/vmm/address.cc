/*
 * Last Modified: Sun May 10 2020
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

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include <cstring>
#include <algorithm>

using vmm::mm_struct;
using vmm::pde_ptr_t;
using vmm::pde_t;
using vmm::vma_struct;

using pmm::boot_mem::boot_alloc_page;

// linked list
using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

uintptr_t V2P(uintptr_t x)
{
    KDEBUG_ASSERT(x >= KERNEL_ADDRESS_SPACE_BASE);
    if (x >= PHYREMAP_VIRTUALBASE && x <= PHYREMAP_VIRTUALEND)
    {
        return x - PHYREMAP_VIRTUALBASE;
    }
    else if (x >= KERNEL_VIRTUALBASE && x <= VIRTUALADDR_LIMIT)
    {
        return ((x)-KERNEL_VIRTUALBASE);
    }
    else
    {
        KDEBUG_RICHPANIC("Invalid address for V2P\n",
                         "KERNEL PANIC: VM",
                         false,
                         "The given address is 0x%x", x);
        return 0;
    }
}

uintptr_t P2V(uintptr_t x)
{
    if (x <= KERNEL_SIZE)
    {
        return P2V_KERNEL(x);
    }
    else if (x >= KERNEL_SIZE && x <= PHYMEMORY_SIZE)
    {
        return P2V_PHYREMAP(x);
    }
    else
    {
        KDEBUG_RICHPANIC("Invalid address for P2V\n",
                         "KERNEL PANIC: VM",
                         false,
                         "The given address is 0x%x", x);
        return 0;
    }
}

uintptr_t P2V_KERNEL(uintptr_t x)
{
    KDEBUG_ASSERT(x <= KERNEL_SIZE);
    return x + KERNEL_VIRTUALBASE;
}

uintptr_t P2V_PHYREMAP(uintptr_t x)
{
    KDEBUG_ASSERT(x <= PHYMEMORY_SIZE);
    return x + PHYREMAP_VIRTUALBASE;
}

uintptr_t IO2V(uintptr_t x)
{
    return P2V(x);
    // KDEBUG_ASSERT(x <= PHYMEMORY_SIZE);
    // return (x - 0xFE000000) + DEVICE_VIRTUALBASE;
}
