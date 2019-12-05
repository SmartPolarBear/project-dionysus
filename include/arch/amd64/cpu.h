#if !defined(__INCLUDE_ARCH_AMD64_CPU_H)
#define __INCLUDE_ARCH_AMD64_CPU_H

#include "sys/types.h"
#include "sys/mmu.h"

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

static inline void ltr(uint16_t sel)
{
    asm volatile("ltr %0"
                 :
                 : "r"(sel));
}

static inline void lgdt(segdesc *p, size_t size)
{
    volatile uint16_t pd[5] = {0};

    pd[0] = size - 1;
    pd[1] = (uintptr_t)p;
    pd[2] = (uintptr_t)p >> 16;
    pd[3] = (uintptr_t)p >> 32;
    pd[4] = (uintptr_t)p >> 48;

    asm volatile("lgdt (%0)"
                 :
                 : "r"(pd));
}

#endif // __INCLUDE_ARCH_AMD64_CPU_H
