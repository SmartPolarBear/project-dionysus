#if !defined(__INCLUDE_ARCH_AMD64_ATOMIC_H)
#define __INCLUDE_ARCH_AMD64_ATOMIC_H

#include "sys/types.h"

template <typename T>
static inline auto xchg(volatile T *addr, T newval) -> T
{
    T result = 0;

    // The + in "+m" denotes a read-modify-write operand.
    asm volatile("lock; xchg %0, %1"
                 : "+m"(*addr), "=a"(result)
                 : "1"((T)newval)
                 : "cc");

    return result;
}

#endif // __INCLUDE_ARCH_AMD64_ATOMIC_H
