#pragma once

#include "system/types.h"
#include "system/kmem.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"
#include "system/vmm.h"

#include "debug/kdebug.h"

#include "kbl/lock/lock_guard.hpp"

namespace memory
{
// flags_ is reserved for future usage
void* kmalloc(size_t sz, [[maybe_unused]] size_t flags);
void kfree(void* ptr);

template<typename T, size_t T_FLAGS = 0>
struct kernel_stl_allocator
{
	using value_type = T;

	template<typename U>
	struct rebind
	{
		typedef kernel_stl_allocator<U> other;
	};

	kernel_stl_allocator() = default;

	template<typename U, size_t U_FLAGS = T_FLAGS>
	constexpr kernel_stl_allocator(const kernel_stl_allocator<U, U_FLAGS>&) noexcept
	{
		// Do nothing
	}

	[[nodiscard]]T* allocate(size_t n)
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
		{
			KDEBUG_GERNERALPANIC_CODE(ERROR_MEMORY_ALLOC);
		}

		if (auto p = static_cast<T*>(kmalloc(n * sizeof(T), T_FLAGS)))
		{
			return p;
		}

		KDEBUG_GERNERALPANIC_CODE(ERROR_MEMORY_ALLOC);
	}

	void deallocate(T* p, size_t n) noexcept
	{

		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
		{
			KDEBUG_GERNERALPANIC_CODE(ERROR_MEMORY_ALLOC);
		}

		kfree(p);
	}
};

template<typename T, size_t T_FLAGS, typename U, size_t U_FLAGS>
bool operator==(const kernel_stl_allocator<T, T_FLAGS>&, const kernel_stl_allocator<U, U_FLAGS>&)
{
	return T_FLAGS == U_FLAGS;
}

template<typename T, size_t T_FLAGS, typename U, size_t U_FLAGS>
bool operator!=(const kernel_stl_allocator<T>&, const kernel_stl_allocator<U>&)
{
	return T_FLAGS != U_FLAGS;
}

} // namespace memory

