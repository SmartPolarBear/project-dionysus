#pragma once

#include "system/types.h"

static inline uintptr_t rcr0()
{
	uintptr_t cr0{ 0 };
	asm volatile ("mov %%cr0, %0" : "=r" (cr0)::"memory");
	return cr0;
}

static inline void lcr0(uintptr_t val)
{
	asm volatile("mov %0,%%cr0"
	:
	: "r"(val));
}

static inline uintptr_t rcr2()
{
	uintptr_t val{ 0 };
	asm volatile("mov %%cr2,%0"
	: "=r"(val));
	return val;
}

static inline uintptr_t rcr3()
{
	uintptr_t cr3{ 0 };
	asm volatile ("mov %%cr3, %0" : "=r" (cr3)::"memory");
	return cr3;
}

static inline void lcr3(uintptr_t val)
{
	asm volatile("mov %0,%%cr3"
	:
	: "r"(val));
}

static inline uintptr_t rcr4()
{
	uintptr_t cr4{ 0 };
	asm volatile ("mov %%cr4, %0" : "=r" (cr4)::"memory");
	return cr4;
}

static inline void lcr4(uintptr_t val)
{
	asm volatile("mov %0,%%cr4"
	:
	: "r"(val));
}

//read eflags
static inline uint32_t read_eflags()
{
	uint64_t eflags{ 0 };
	asm volatile("pushf; pop %0"
	: "=r"(eflags));
	return static_cast<uint32_t>(eflags & 0xFFFFFFFF);
}

static inline uint64_t read_rflags()
{
	uint64_t rflags{ 0 };
	asm volatile("pushfq; pop %0"
	: "=r"(rflags));
	return rflags;
}