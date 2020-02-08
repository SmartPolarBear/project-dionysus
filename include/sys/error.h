#if !defined(__INCLUDE_SYS_ERROR_H)
#define __INCLUDE_SYS_ERROR_H

#include "sys/types.h"

using RESULT = uint64_t;

enum ErrorCode : RESULT
{
    ERROR_SUCCESS,
    ERROR_UNKOWN,

    ERROR_MEMORY_ALLOC,
    ERROR_REMAP,
    ERROR_VMA_NOT_FOUND_IN_MM,
    ERROR_PAGE_NOT_PERSENT
};

#endif // __INCLUDE_SYS_ERROR_H
