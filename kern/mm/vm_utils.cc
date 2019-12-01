

#include "drivers/debug/kdebug.h"
#include "sys/memlayout.h"

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
        KDEBUG_GENERALPANIC("Invalid address for V2P\n");
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
        KDEBUG_GENERALPANIC("Invalid address for P2V\n");
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