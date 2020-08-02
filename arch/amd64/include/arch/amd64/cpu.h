#pragma once
#include "system/mmu.h"
#include "system/types.h"

#include "system/segmentation.hpp"

extern "C" void load_gdt(gdt_table_desc *gdt_ptr);
extern "C" void load_idt(idt_table_desc *idt_ptr);
extern "C" void load_tr(size_t seg);

extern "C" void safe_swap_gs();
extern "C" void swap_gs();

extern "C" void cli();
extern "C" void sti();
extern "C" void hlt();
