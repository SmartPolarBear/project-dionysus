/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:14
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-13 22:53:41
 * @ Description:
 */
#if !defined(__INCLUDE_SYS_MEMLAYOUT_H)
#define __INCLUDE_SYS_MEMLAYOUT_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

constexpr uintptr_t KERNEL_VIRTUALBASE = 0xFFFFFFFF80000000;
constexpr uintptr_t KERNEL_VIRTUALLINK = 0xFFFFFFFF80100000;
constexpr uintptr_t DEVICE_VIRTUALBASE = 0xFFFFFFFF40000000;

constexpr uintptr_t DEVICE_PHYSICALBASE = 0xFE000000;


template <typename P>
static inline P *V2P(void *a)
{
    return (P *)(((uintptr_t)(a)) - KERNEL_VIRTUALBASE);
}

template <typename P>
static inline P *P2V(void *a)
{
    return (P *)(((void *)(((char *)(a)) + KERNEL_VIRTUALBASE)));
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
