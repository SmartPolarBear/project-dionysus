#pragma once
#include "system/types.h"

struct arch_task_context_registers
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

struct arch_pseudo_descriptor
{
	uint16_t limit;
	uint64_t base;
} __attribute__((__packed__));


extern "C" void load_gdt(arch_pseudo_descriptor* gdt_ptr);
extern "C" void load_idt(arch_pseudo_descriptor* idt_ptr);
extern "C" void load_tr(size_t seg);

extern "C" void swap_gs();

extern "C" void cli();
extern "C" void sti();
extern "C" void hlt();

extern "C" void enable_avx();
