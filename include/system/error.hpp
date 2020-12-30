#pragma once

#include "system/types.h"

#include <variant>

// declaration to avoid fucking circle reference of headers
namespace kdebug
{
	void kdebug_warning(const char* fmt, ...);
}

// use negative value to indicate errors

enum error_code_values : error_code
{
	ERROR_SUCCESS,                // the action is completed successfully
	ERROR_UNKOWN,                // failed, but reason can't be figured out
	ERROR_INVALID,                    // invalid kbl
	ERROR_NOT_IMPL,                // not implemented
	ERROR_LOCK_STATUS,            // lock is not at a right status
	ERROR_UNSUPPORTED,                // unsupported features
	ERROR_MEMORY_ALLOC,            // insufficient memory
	ERROR_REWRITE,                    // rewrite the kbl that shouldn't be done so
	ERROR_VMA_NOT_FOUND,            // can't find a VMA
	ERROR_PAGE_NOT_PRESENT,        // page isn't persent
	ERROR_HARDWARE_NOT_COMPATIBLE,    // the hardware isn't compatible with the kernel
	ERROR_TOO_MANY_PROC,            // too many child_processes
	ERROR_CANNOT_WAKEUP,            // task's state isn't valid for waking up
	ERROR_HAS_KILLED,               // task to be killed has been killed
	ERROR_BUSY,                    // device is busy
	ERROR_DEV_TIMEOUT,                // device operation time out
	ERROR_IO,                        // IO error
	ERROR_OBSOLETE,          // Out of date
	ERROR_OUT_OF_BOUND,
	ERROR_INTERNAL,
	ERROR_IS_DIR,
	ERROR_NOT_DIR,
	ERROR_DEV_NOT_FOUND,
	ERROR_NO_ENTRY,
	ERROR_EOF,
	ERROR_INVALID_ACCESS,
	ERROR_SHOULD_NOT_REACH_HERE,
	ERROR_TOO_MANY_CALLS,
	ERROR_ACCESS,
	ERROR_NOT_EXIST,
	ERROR_ALREADY_EXIST,
	ERROR_NOT_FILE,
	ERROR_PERMISSION,

	// max element index
	ERROR_CODE_COUNT,
};


template<typename T>
using error_code_with_result = std::variant<error_code, T>;

template<typename T>
static inline bool has_error(error_code_with_result<T> ret) noexcept
{
	if (std::holds_alternative<error_code>(ret) && std::get<error_code>(ret) == ERROR_SUCCESS)
	{
		kdebug::kdebug_warning("has_error: ret contains a ERROR_SUCCESS.\nThis may reveal a misuse.\n");
		return false;
	}
	return std::holds_alternative<error_code>(ret);
}

template<typename T>
static inline error_code get_error_code(error_code_with_result<T> ret) noexcept
{
	return std::holds_alternative<error_code>(ret)
		   ? std::get<error_code>(ret)
		   : ERROR_SUCCESS;
}

template<typename T>
static inline T get_result(error_code_with_result<T> res) noexcept
{
	return std::get<T>(res);
}