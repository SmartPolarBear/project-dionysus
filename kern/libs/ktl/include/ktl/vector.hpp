#pragma once

#include "system/kmalloc.hpp"

#include <vector>

namespace ktl
{
	using std::vector;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_vector = std::vector<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;
}