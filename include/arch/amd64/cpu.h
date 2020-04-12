#pragma once
#include "sys/mmu.h"
#include "sys/types.h"

#include "sys/segmentation.hpp"

extern "C" void load_gdt_and_tr(gdt_table_ptr *gdt_ptr, uint64_t tss_sel);

extern "C" void safe_swap_gs();
extern "C" void swap_gs();

extern "C" void cli();
extern "C" void sti();
extern "C" void hlt();

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
