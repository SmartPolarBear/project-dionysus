#pragma once
#include "system/kmalloc.hpp"

#include <stack>
#include <ktl/queue.hpp>

namespace ktl
{
	using std::stack;

	template<typename T, size_t MEM_FLAGS = 0>
	using ktl_stack = std::stack<T, ktl_deque<T, MEM_FLAGS>>;
}