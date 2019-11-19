#if !defined(__INCLUDE_SYS_ERROR_H)
#define __INCLUDE_SYS_ERROR_H

#include "sys/types.h"

using RESULT = uint64_t;

enum ErrorCode : RESULT
{
    ERROR_SUCCESS = 0,
    ERROR_MEMORY_ALLOC = 1
};

#endif // __INCLUDE_SYS_ERROR_H
