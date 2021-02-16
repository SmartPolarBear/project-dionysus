#pragma once
#include "ktl/concepts.hpp"

namespace lock
{
/// \brief the concept that constraints the name_ requirement BasicLockable
/// https://en.cppreference.com/w/cpp/named_req/BasicLockable
/// \tparam
template<typename T>
concept BasicLockable=
requires(T lk){ lk.lock();lk.unlock(); };

template<typename T>
concept Lockable= BasicLockable<T> &&
	requires(T t)
	{
		{ t.try_lock() }->ktl::convertible_to<bool>;
	};

}
