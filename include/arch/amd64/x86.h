#pragma once

#include "sys/types.h"

// cpuid and corrosponding enumerations
#include "arch/amd64/cpuid.h"

// read and write specific registers
#include "arch/amd64/regs.h"

// read and write msr
#include "arch/amd64/msr.h"

// cpu features like halt and interrupt enability
#include "arch/amd64/cpu.h"

// atomic
#include "arch/amd64/atomic.h"

// port io and SIMD

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

static inline void outsl(int port, const void *addr, int cnt)
{
    asm volatile("cld; rep outsl"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

static inline void stosb(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosb"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

static inline void stosl(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosl"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

static inline void invlpg(void *addr)
{
    asm volatile("invlpg (%0)" ::"r"(addr)
                 : "memory");
}
