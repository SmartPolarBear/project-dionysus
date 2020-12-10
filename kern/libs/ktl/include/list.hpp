#pragma once
#include "system/kmalloc.hpp"

#include <list>

namespace ktl
{
	using std::list;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_list = std::list<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;
}