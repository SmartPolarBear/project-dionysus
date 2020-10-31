#pragma once

#include "system/kmalloc.hpp"

#include <list>
#include <vector>
#include <queue>
#include <forward_list>

template<typename T, size_t MEM_FLAGS = 0>
using stl_vector = std::vector<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using stl_deque = std::deque<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using stl_list = std::list<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS= 0>
using stl_forward_list = std::forward_list<T, memory::kernel_stl_allocator<T, MEM_FLAGS>>;