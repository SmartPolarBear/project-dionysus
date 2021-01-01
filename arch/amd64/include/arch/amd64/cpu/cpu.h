#pragma once
#include "system/mmu.h"
#include "system/types.h"

#include "system/segmentation.hpp"

struct arch_context_registers
{
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip; //rip will be pop automatically by ret instruction
}__attribute__((packed));


extern "C" void load_gdt(gdt_table_desc *gdt_ptr);
extern "C" void load_idt(idt_table_desc *idt_ptr);
extern "C" void load_tr(size_t seg);

extern "C" void swap_gs();

extern "C" void cli();
extern "C" void sti();
extern "C" void hlt();

extern "C" void enable_avx();
