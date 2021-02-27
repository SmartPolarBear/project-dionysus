#include "object/kernel_object.hpp"

using namespace object;

ktl::atomic<size_t> basic_object::obj_counter_{ 0 };

// if not defined outline, the constructor isn't called
koid_allocator& koid_allocator::instance()
{
	static koid_allocator alloc{ 1 };
	return alloc;
}

