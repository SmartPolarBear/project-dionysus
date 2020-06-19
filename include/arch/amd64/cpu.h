#pragma once
#include "system/mmu.h"
#include "system/types.h"

#include "system/segmentation.hpp"

extern "C" void load_gdt_and_tr(gdt_table_desc *gdt_ptr, uint64_t tss_sel);
extern "C" void load_idt(idt_table_desc *idt_ptr);

extern "C" void safe_swap_gs();
extern "C" void swap_gs();

extern "C" void cli();
extern "C" void sti();
extern "C" void hlt();
