#pragma once

#include "system/types.h"


#include <variant>

// use negative value to indicate errors

enum error_code_values : error_code
{
	ERROR_SUCCESS,                // the action is completed successfully
	ERROR_UNKOWN,                // failed, but reason can't be figured out
	ERROR_INVALID,                    // invalid data
	ERROR_NOT_IMPL,                // not implemented
	ERROR_LOCK_STATUS,            // lock is not at a right status
	ERROR_UNSUPPORTED,                // unsupported features
	ERROR_MEMORY_ALLOC,            // insufficient memory
	ERROR_REWRITE,                    // rewrite the data that shouldn't be done so
	ERROR_VMA_NOT_FOUND,            // can't find a VMA
	ERROR_PAGE_NOT_PRESENT,        // page isn't persent
	ERROR_HARDWARE_NOT_COMPATIBLE,    // the hardware isn't compatible with the kernel
	ERROR_TOO_MANY_PROC,            // too many processes
	ERROR_CANNOT_WAKEUP,            // process's state isn't valid for waking up
	ERROR_HAS_KILLED,               // process to be killed has been killed
	ERROR_DEV_BUSY,                    // device is busy
	ERROR_DEV_TIMEOUT,                // device operation time out
	ERROR_IO,                        // IO error
};


// this should be in the global namespace
using error_code = int64_t;

template<typename T>
using error_code_with_result = std::variant<error_code, T>;

template<typename T>
static inline bool get_error_code(error_code_with_result<T> ret)
{
	return std::holds_alternative<error_code>(ret)
		   ? std::get<error_code>(ret)
		   : ERROR_SUCCESS;
}
