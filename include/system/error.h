#pragma once

#include "system/types.h"

// use negative value to indicate errors

enum error_code_values : error_code
{
    ERROR_SUCCESS,      // the action is completed successfully
    ERROR_UNKOWN,       // failed, but reason can't be figured out
    ERROR_INVALID_DATA, // invalid data
    ERROR_NOT_IMPL,     // not implemented
    ERROR_INVALID_ARG,  // invalid args
    ERROR_LOCK_STATUS,  // lock is not at a right status

    ERROR_MEMORY_ALLOC,           // insufficient memory
    ERROR_INVALID_ADDR,           // invalid address
    ERROR_REWRITE,                // rewrite the data that shouldn't be done so
    ERROR_VMA_NOT_FOUND,          // can't find a VMA
    ERROR_PAGE_NOT_PERSENT,       // page isn't persent
    ERROR_HARDWARE_NOT_COMPATIBLE, // the hardware isn't compatible with the kernel
    ERROR_TOO_MANY_PROC,          // too many processes
    ERROR_CANNOT_WAKEUP,            // process's state isn't valid for waking up
    ERROR_HAS_KILLED,               // process to be killed has been killed
};

// this should be in the global namespace
using error_code = int64_t;

