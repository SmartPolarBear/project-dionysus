#pragma once

#include "system/types.h"

#include <immintrin.h>
#include <x86intrin.h>

namespace arch
{

[[maybe_unused]]static inline void cpu_yield()
{
	_mm_pause();
}

[[maybe_unused]]static inline void mfence()
{
	asm volatile("mfence":: :"memory");
}

[[maybe_unused]]static size_t cycles()
{
	return _rdtsc();
}

}