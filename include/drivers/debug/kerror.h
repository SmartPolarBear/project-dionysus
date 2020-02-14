#if !defined(__INCLUDE_DRIVERS_DEBUG_KERRORS_H)
#define __INCLUDE_DRIVERS_DEBUG_KERRORS_H

#include "sys/types.h"

#include "drivers/debug/hresult.h"

enum hresult_value : hresult
{
    ERROR_SUCCESS,      // the action is completed successfully
    ERROR_UNKOWN,       // failed, but reason can't be figured out
    ERROR_INVALID_DATA, // invalid data

    ERROR_MEMORY_ALLOC,    // insufficient memory
    ERROR_REWRITE,         // rewrite the data that shouldn't be done so
    ERROR_VMA_NOT_FOUND,   // can't find a VMA
    ERROR_PAGE_NOT_PERSENT // page isn't persent
};

#endif // __INCLUDE_DRIVERS_DEBUG_KERRORS_H
