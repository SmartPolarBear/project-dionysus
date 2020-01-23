/**
 * @ Author: SmartPolarBear
 * @ Create Time: 1970-01-01 08:00:00
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-16 14:34:57
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_TYPES_H)
#define __INCLUDE_SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

using ldbl = long double;
using ull = unsigned long long;

constexpr auto STORAGE_UNIT = 1024ULL;

static inline constexpr ldbl operator"" _KB(ldbl sz)
{
    return sz * STORAGE_UNIT;
}

static inline constexpr ull operator"" _KB(ull sz)
{
    return sz * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" _MB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator"" _MB(ull sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" _GB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator"" _GB(ull sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator"" _TB(ldbl sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator"" _TB(ull sz)
{
    return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

template <typename T>
static inline auto powerof2_roundup(T x) -> T
{
    return 1ull << (sizeof(T) * 8 - __builtin_clzll(x));
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr)-offsetof(type, member)))

// linked list head
struct list_head
{
    list_head *next, *prev;
};

using list_foreach_func = void (*)(list_head *);

#endif // __INCLUDE_SYS_TYPES_H
