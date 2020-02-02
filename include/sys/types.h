/*
 * Last Modified: Sun Feb 02 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#if !defined(__INCLUDE_SYS_TYPES_H)
#define __INCLUDE_SYS_TYPES_H

// va_list and some things
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// std::log2p1
#include <bit>

using std::log2p1;

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

// round down to the nearest multiple of n
template <typename T>
static inline constexpr T rounddown(T val, T n)
{
    return val - val % n;
}

// round up to the nearest multiple of n
template <typename T>
static inline constexpr T roundup(T val, T n)
{
    return rounddown(a + n - 1, n);
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
