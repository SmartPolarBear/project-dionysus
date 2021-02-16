#pragma once

#include "system/types.h"

// cpuid and corrosponding enumerations
#include "cpuid.h"

// read and write specific registers
#include "regs.h"

// read and write msr
#include "msr.h"

// cpu features like halt and interrupt enability
#include "cpu.h"

// atomic
#include "atomic.h"

// port io
#include "port_io.h"


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

