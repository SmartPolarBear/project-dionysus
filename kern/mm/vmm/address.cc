
#include "sys/vmm.h"
#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/memory.h"
#include "sys/mmu.h"
#include "sys/pmm.h"

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
        KDEBUG_RICHPANIC("Invalid address for V2P\n",
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
    KDEBUG_ASSERT(x <= PHYMEMORY_SIZE);
    return (x - 0xFE000000) + DEVICE_VIRTUALBASE;
}
