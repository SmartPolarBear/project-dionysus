#pragma once
#include "system/kmalloc.hpp"

#include <queue>
#include <vector.hpp>


namespace ktl
{
	using std::deque;
	using std::queue;
	using std::priority_queue;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_deque = std::deque<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_priority_queue = std::priority_queue<T, ktl_vector<T, MEM_FLAGS>>;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_queue = std::queue<T, ktl_deque<T, MEM_FLAGS>>;

}