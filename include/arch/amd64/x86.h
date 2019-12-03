#if !defined(__INCLUDE_ARCH_X86_H)
#define __INCLUDE_ARCH_X86_H

#if !defined(__cplusplus)
#error Only available for C++
#endif

#include "sys/types.h"

static inline uint8_t inb(uint16_t port)
{
    uint8_t data = 0;
    asm volatile("in %1,%0"
                 : "=a"(data)
                 : "d"(port));

    return data;
}

static inline void outb(uint16_t port, uint8_t data)
{
    asm volatile("out %0,%1"
                 :
                 : "a"(data), "d"(port));
}

static inline void hlt(void)
{
    asm volatile("hlt");
}

static inline void cli(void)
{
    asm volatile("cli");
}

static inline void sti(void)
{
    asm volatile("sti");
}

static inline void
outsl(int port, const void *addr, int cnt)
{
    asm volatile("cld; rep outsl"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

static inline void
stosb(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosb"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosl"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

static inline uintptr_t rcr2(void)
{
    uintptr_t val;
    asm volatile("mov %%cr2,%0"
                 : "=r"(val));
    return val;
}

static inline void lcr3(uintptr_t val)
{
    asm volatile("mov %0,%%cr3"
                 :
                 : "r"(val));
}


#endif // __INCLUDE_ARCH_X86_H
