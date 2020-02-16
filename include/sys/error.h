#if !defined(__INCLUDE_SYS_ERROR_H)
#define __INCLUDE_SYS_ERROR_H

#include "sys/types.h"

// use negative value to indicate errors

enum error_code_values : error_code
{
    ERROR_SUCCESS,      // the action is completed successfully
    ERROR_UNKOWN,       // failed, but reason can't be figured out
    ERROR_INVALID_DATA, // invalid data

    ERROR_MEMORY_ALLOC,    // insufficient memory
    ERROR_REWRITE,         // rewrite the data that shouldn't be done so
    ERROR_VMA_NOT_FOUND,   // can't find a VMA
    ERROR_PAGE_NOT_PERSENT // page isn't persent
};

#include "sys/types.h"

// this should be in the global namespace
using error_code = int64_t;


#endif // __INCLUDE_SYS_ERROR_H
