#pragma once

#include "system/kmalloc.hpp"

#include <list>
#include <vector>
#include <queue>
#include <forward_list>

template<typename T, size_t MEM_FLAGS = 0>
using kvector = std::vector<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using kdeque = std::deque<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using klist = std::list<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using kforward_list = std::forward_list<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;