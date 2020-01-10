#if !defined(__INCLUDE_ARCH_AMD64_ATOMIC_H)
#define __INCLUDE_ARCH_AMD64_ATOMIC_H

#include "sys/types.h"

static inline uint32_t xchg(volatile uint32_t *addr, size_t newval)
{
    uint32_t result;

    // The + in "+m" denotes a read-modify-write operand.
    asm volatile("lock xchgl %0, %1"
                 : "+m"(*addr), "=a"(result)
                 : "1"((uint32_t)newval)
                 : "cc");
    return result;
}

#endif // __INCLUDE_ARCH_AMD64_ATOMIC_H
