#if !defined(__INCLUDE_SYS_ERROR_H)
#define __INCLUDE_SYS_ERROR_H

#include "sys/types.h"

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

//HRESULT: refer to https://docs.microsoft.com/en-us/previous-versions/bb446131(v=msdn.10)?redirectedfrom=MSDN
//Possible values: refer to https://docs.microsoft.com/en-us/previous-versions/aa914935%28v%3dmsdn.10%29

#include "sys/types.h"

// this should be in the global namespace
using hresult = int64_t;

static inline constexpr bool FAILED(hresult hr)
{
    return hr < 0;
}

static inline constexpr int64_t HRESULT_CODE(hresult hr)
{
    return hr & 0b1111111111111111;
}

static inline constexpr int64_t HRESULT_FACILITY(hresult hr)
{
    return (hr >> 16) & 0b11111111111;
}

#endif // __INCLUDE_SYS_ERROR_H
