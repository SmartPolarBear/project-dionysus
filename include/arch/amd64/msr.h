#if !defined(__INCLUDE_ARCH_AMD64_MSR_H)
#define __INCLUDE_ARCH_AMD64_MSR_H

#include <sys/types.h>

// not all MSRs are listed. only a few.
enum MSR_REGISTERS
{
    MSR_EFER = 0xc0000080,           // extended feature register
    MSR_STAR = 0xc0000081,           // legacy mode SYSCALL target
    MSR_LSTAR = 0xc0000082,          // long mode SYSCALL target
    MSR_CSTAR = 0xc0000083,          // compat mode SYSCALL target
    MSR_SYSCALL_MASK = 0xc0000084,   // EFLAGS mask for syscall
    MSR_FS_BASE = 0xc0000100,        // 64bit FS base
    MSR_GS_BASE = 0xc0000101,        // 64bit GS base
    MSR_KERNEL_GS_BASE = 0xc0000102, // SwapGS GS shadow
};

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
