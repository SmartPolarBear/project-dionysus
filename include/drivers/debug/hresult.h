#if !defined(__INCLUDE_DRIVERS_DEBUG_HRESULT_H)
#define __INCLUDE_DRIVERS_DEBUG_HRESULT_H

//HRESULT: refer to https://docs.microsoft.com/en-us/previous-versions/bb446131(v=msdn.10)?redirectedfrom=MSDN
//Possible values: refer to https://docs.microsoft.com/en-us/previous-versions/aa914935%28v%3dmsdn.10%29

#include "sys/types.h"

// this should be in the global namespace
using hresult = int64_t;

#include "drivers/debug/kerror.h"


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

#endif // __INCLUDE_DRIVERS_DEBUG_HRESULT_H
