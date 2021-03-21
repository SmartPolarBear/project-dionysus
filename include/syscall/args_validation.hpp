#pragma once

#include "syscall/syscall.hpp"

#include "debug/kdebug.h"

#include "ktl/concepts.hpp"
#include "ktl/algorithm.hpp"

#include "system/memlayout.h"

namespace syscall
{
template<typename T>
requires ktl::convertible_to<T, uintptr_t> || ktl::is_pointer<T>
bool arg_valid_pointer(T arg)
{
	uintptr_t ptr = 0;
	if constexpr (ktl::is_pointer<T>)
	{
		ptr = reinterpret_cast<uintptr_t>(arg);
	}
	else
	{
		ptr = static_cast<uintptr_t>(arg);
	}

	return VALID_USER_PTR(ptr);
}

constexpr size_t USER_STR_MAX = INT32_MAX;

/// \brief Check if arg is pointer to a null-terminated string
/// \tparam T
/// \param arg
/// \return
template<typename T>
requires ktl::convertible_to<T, uintptr_t> || ktl::is_pointer<T>
bool arg_valid_string(T arg)
{
	uintptr_t ptr = 0;
	if constexpr (ktl::is_pointer<T>)
	{
		ptr = reinterpret_cast<uintptr_t>(arg);
	}
	else
	{
		ptr = static_cast<uintptr_t>(arg);
	}

	auto str = reinterpret_cast<char*>(ptr), begin = reinterpret_cast<char*>(ptr);
	size_t len = 0;
	for (str; VALID_USER_PTR(str) && (*str) && static_cast<uint64_t>(len = str - begin) < USER_STR_MAX;
	     str++)
	{
		if (!VALID_USER_PTR(str))
		{
			return false;
		}
	}

	if (len >= USER_STR_MAX)
	{
		return false;
	}

	return VALID_USER_PTR(str);
}

}