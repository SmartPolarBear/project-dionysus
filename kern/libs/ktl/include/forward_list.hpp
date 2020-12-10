#pragma once
#include "system/kmalloc.hpp"

#include <forward_list>

namespace ktl
{

	using std::forward_list;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_forward_list = std::forward_list<T, memory::kernel_stl_allocator < T, MEM_FLAGS>>;

}