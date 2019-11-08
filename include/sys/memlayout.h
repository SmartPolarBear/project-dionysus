/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:14
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-07 23:27:54
 * @ Description:
 */
#if !defined(__INCLUDE_SYS_MEMLAYOUT_H)
#define __INCLUDE_SYS_MEMLAYOUT_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

constexpr auto STORAGE_UNIT = 1024E1L;
using ldbl = long double;

static inline constexpr ldbl operator"" KB(ldbl sz)
{
    return sz * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" MB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" GB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" TB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

// the max value for a valid address
constexpr uintptr_t VIRTUALADDR_LIMIT = 0xFFFFFFFFFFFFFFFF;
// first address for kernel
constexpr uintptr_t KERNADDRSPACE_BASE = 0xFFFF800000000000;


// remap of physical memory
constexpr uintptr_t PHYREMAP_VIRTUALBASE = 0xffff888000000000;
constexpr uintptr_t PHYREMAP_VIRTUALEND = 0xffffc87fffffffff;
constexpr size_t PHYMEMORY_SIZE = PHYREMAP_VIRTUALEND - PHYREMAP_VIRTUALBASE + 1;

// map kernel, from physical address 0 to 2GiB
constexpr uintptr_t KERNEL_VIRTUALBASE = 0xFFFFFFFF80000000;
constexpr uintptr_t KERNEL_VIRTUALEND = VIRTUALADDR_LIMIT;
constexpr uintptr_t KERNEL_SIZE = KERNEL_VIRTUALEND - KERNEL_VIRTUALBASE + 1;
// Note that the multiboot info will be placed just after kernel
// Be greatly cautious not to overwrite it !!!!
constexpr uintptr_t KERNEL_VIRTUALLINK = 0xFFFFFFFF80100000;

// for memory-mapped IO
constexpr uintptr_t DEVICE_VIRTUALBASE = 0xFFFFFFFF40000000;
constexpr uintptr_t DEVICE_PHYSICALBASE = 0xFE000000;

template <typename P>
static inline P V2P_WO(P a)
{
    return (P)(((uintptr_t)(a)) - KERNEL_VIRTUALBASE);
}

template <typename P>
static inline P P2V_WO(P a)
{
    return (P)(((void *)(((char *)(a)) + KERNEL_VIRTUALBASE)));
}

template <typename P>
static inline P V2P(void *a)
{
    return (P)(((uintptr_t)(a)) - KERNEL_VIRTUALBASE);
}

template <typename P>
static inline P P2V(void *a)
{
    return (P)(((void *)(((char *)(a)) + KERNEL_VIRTUALBASE)));
}

static inline constexpr uintptr_t V2P(uintptr_t x)
{
    return ((x)-KERNEL_VIRTUALBASE);
}

static inline constexpr uintptr_t P2V(uintptr_t x)
{
    return ((x) + KERNEL_VIRTUALBASE);
}

#endif // __INCLUDE_SYS_MEMLAYOUT_H
