#pragma once
// Imported from zircon kernel
// Previous license:
//
// Copyright 2018 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <memory>

#include "kbl/checker/allocate_checker.hpp"
#include "ktl/type_traits.hpp"
#include "ktl/concepts.hpp"

namespace ktl
{

using std::unique_ptr;

//ktl::enable_if_t<!ktl::is_array_v<T>, unique_ptr<T>>
template<typename T, typename... Args>
requires ktl::is_array_v<T>

static inline unique_ptr<T> make_unique(kbl::allocate_checker* ac, Args&& ... args)
{
	return unique_ptr<T>(new(ac) T(std::forward<Args>(args)...));
}

//// For an unbounded array of given size, each element is default-constructed.
//// This is different from plain `new (ac) T[n]`, which leaves it uninitialized.
//ktl::enable_if_t<ktl::is_unbounded_array_v<T>, unique_ptr<T>>
template<typename T, typename... Args>
requires ktl::is_unbounded_array_v<T>
static inline unique_ptr<T> make_unique(kbl::allocate_checker* ac, size_t n)
{
	return unique_ptr<T>(new(ac) ktl::remove_extent_t<T>[n]());
}

}  // namespace ktl
