#pragma once
#include "system/types.h"

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

static inline void outw(uint16_t addr, uint16_t v)
{
	asm volatile("outw %0, %1"::"a"(v), "Nd"(addr));
}

static inline uint16_t inw(uint16_t addr)
{
	uint16_t v;
	asm volatile("inw %1, %0":"=a"(v):"Nd"(addr));
	return v;
}

static inline void outl(uint16_t addr, uint32_t v)
{
	asm volatile("outl %0, %1"::"a"(v), "Nd"(addr));
}

static inline uint32_t inl(uint16_t addr)
{
	uint32_t v;
	asm volatile("inl %1, %0":"=a"(v):"Nd"(addr));
	return v;
}
