#pragma once

#include <memory>

#include "kbl/checker/allocate_checker.hpp"
#include "ktl/type_traits.hpp"

namespace ktl
{
	using std::shared_ptr;

	template<typename T, typename... Args>
	static inline ktl::enable_if_t<!ktl::is_array_v<T>, shared_ptr<T>> make_shared(kbl::allocate_checker* ac,
		Args&& ... args)
	{
		return shared_ptr<T>(new(ac) T(std::forward<Args>(args)...));
	}

//// For an unbounded array of given size, each element is default-constructed.
//// This is different from plain `new (ac) T[n]`, which leaves it uninitialized.
	template<typename T, typename... Args>
	static inline ktl::enable_if_t<ktl::is_unbounded_array_v<T>, shared_ptr<T>> make_shared(kbl::allocate_checker* ac,
		size_t n)
	{
		return shared_ptr<T>(new(ac) ktl::remove_extent_t<T>[n]());
	}
}