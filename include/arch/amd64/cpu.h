#pragma once
#include "sys/mmu.h"
#include "sys/types.h"

#include "sys/segmentation.hpp"

extern "C" void load_gdt(gdt_table_ptr *gdt_ptr);
extern "C" void safe_swap_gs();
extern "C" void swap_gs();

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

static inline void lgdt(gdt_table_ptr *gdt_ptr)
{
    asm volatile("lgdt (%0)"
                 :
                 : "r"(gdt_ptr));
}

static inline void lidt(uintptr_t p, int size)
{
    volatile uint16_t pd[5];

    pd[0] = size - 1;
    pd[1] = p;
    pd[2] = p >> 16;
    pd[3] = p >> 32;
    pd[4] = p >> 48;

    asm volatile("lidt (%0)"
                 :
                 : "r"(pd));
}
