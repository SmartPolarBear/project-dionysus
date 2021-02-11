#pragma once

#include "system/types.h"

namespace kbl
{

using canary_magic_type = uint32_t;

static inline constexpr canary_magic_type INVALID_MAGIC_VAL = UINT32_MAX;

namespace internal
{
static inline constexpr bool valid_magic(const char* str)
{
	return str[0] != '\0' && str[1] != '\0' && str[2] != '\0' && str[3] != '\0' && str[4] == '\0';
}
}

static inline constexpr canary_magic_type magic(const char* str)
{
	if (!internal::valid_magic(str))
	{
		return INVALID_MAGIC_VAL;
	}

	canary_magic_type res = 0;
	for (size_t i = 0; i < 4; ++i)
	{
		res = (res << 8) + str[i];
	}
	return res;
}

template<canary_magic_type Magic>
class canary
{
 public:
	static_assert(Magic != INVALID_MAGIC_VAL);

	[[nodiscard]] constexpr canary() : magic_(Magic)
	{
	}

	~canary()
	{
		assert();
		magic_ = 0;
	}

	void assert()
	{
		KDEBUG_ASSERT(valid());
	}

	[[nodiscard]] bool valid() const
	{
		return magic_ == Magic;
	}

 private:
	volatile canary_magic_type magic_;
};

}