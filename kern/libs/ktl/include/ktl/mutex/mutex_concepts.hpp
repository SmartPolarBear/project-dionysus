#pragma once
#include <concepts>

namespace ktl
{
	namespace mutex
	{
		/// \brief the concept that constraints the name_ requirement BasicLockable
		/// https://en.cppreference.com/w/cpp/named_req/BasicLockable
		/// \tparam
		template<typename T>
		concept BasicLockable=
		requires(T lk){ lk.lock();lk.unlock(); };
	}
}