#pragma once

#include "system/types.h"

#include "kbl/lock/spinlock.h"

#include "ktl/algorithm.hpp"
#include "ktl/string_view.hpp"
#include "ktl/span.hpp"
#include "kbl/lock/lock_guard.hpp"

namespace kbl
{
template<size_t Size>
class name
{
 public:
	static_assert(Size >= 1u, "names must be longer than 1");

	name() = default;

	~name() = default;

	explicit name(ktl::string_view sv)  TA_REQ(!lk_)
	{
		set(sv);
	}

	explicit name(ktl::span<char> sp)  TA_REQ(!lk_)
	{
		set(sp);
	}

	name(const char* name, size_t len)  TA_REQ(!lk_)
	{
		set(name, len);
	}

	const char* data() const TA_REQ(!lk_)
	{
		lock::lock_guard g{ lk_ };
		return name_;
	}

	void set(const char* name, size_t len)  TA_REQ(!lk_)
	{
		lock::lock_guard g{ lk_ };
		strncpy(name_, name, ktl::min(len, Size));
	}

	void set(ktl::string_view sv)  TA_REQ(!lk_)
	{
		lock::lock_guard g{ lk_ };

		ktl::copy(sv.begin(), sv.end(), name_);
	}

	void set(ktl::span<char> sp)  TA_REQ(!lk_)
	{
		lock::lock_guard g{ lk_ };

		ktl::copy(sp.begin(), sp.end(), name_);
	}

 private:
	mutable lock::spinlock lk_{ "name_" };
	char name_[Size + 1] TA_GUARDED(lk_) = { 0 };
};
}