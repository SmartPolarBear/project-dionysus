#pragma once

#include "system/types.h"

#include <immintrin.h>
#include <x86intrin.h>

namespace arch
{

static inline void cpu_yield()
{
	_mm_pause();
}

static inline void mfence()
{
	asm volatile("mfence":: :"memory");
}

static size_t cycles()
{
	return _rdtsc();
}

}