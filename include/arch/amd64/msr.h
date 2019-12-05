#if !defined(__INCLUDE_ARCH_AMD64_MSR_H)
#define __INCLUDE_ARCH_AMD64_MSR_H

#include <sys/types.h>


static inline void wrmsr(uint64_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    asm volatile(
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint64_t msr)
{
    uint32_t low, high;
    asm volatile(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

#endif // __INCLUDE_ARCH_AMD64_MSR_H
