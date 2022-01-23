#pragma once
#include "system/types.h"

namespace object
{
using handle_type = uint64_t;
static inline constexpr handle_type INVALID_HANDLE_VALUE = 0;

static inline constexpr uint64_t HANDLE_ATTRIBUTE(handle_type handle)
{
	return handle >> 32u;
}

static inline constexpr bool HANDLE_HAS_ATTRIBUTE(handle_type handle, uint64_t attributes)
{
	return (HANDLE_ATTRIBUTE(handle) & attributes) == attributes;
}

}