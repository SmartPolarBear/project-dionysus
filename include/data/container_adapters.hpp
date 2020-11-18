#pragma once
#include "data/basic_containers.hpp"

#include <stack>
#include <queue>

template<typename T, size_t MEM_FLAGS>
using stl_stack = std::stack<T, stl_deque<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS>
using stl_queue = std::queue<T, stl_deque<T, MEM_FLAGS>>;

template<typename T, size_t MEM_FLAGS>
using stl_priority_queue = std::priority_queue<T, stl_vector<T, MEM_FLAGS>>;