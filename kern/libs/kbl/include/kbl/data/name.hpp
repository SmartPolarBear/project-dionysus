#pragma once

#include "system/types.h"

#include "kbl/lock/spinlock.h"

namespace kbl
{
template<size_t Size>
class name
{
 public:
	static_assert(Size >= 1u, "names must be longer than 1");

	name() = default;

	~name() = default;

	name(const char* name, size_t len)
	{

	}

 private:
	mutable lock::spinlock lk_{ "name" };
	char name_[Size] TA_GUARDED(lk_) = { 0 };
};
}