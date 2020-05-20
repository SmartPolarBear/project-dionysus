/*
 * Last Modified: Sat Mar 07 2020
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

#pragma once

// va_list and some things
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define PANIC
#define OUT
#define IN
#define OPTIONAL

constexpr size_t log2(size_t n)
{
    return ((n < 2) ? 1 : 1 + log2(n / 2));
}

constexpr size_t log2p1(size_t n)
{
    return (log2(n)) + 1;
}

using ldbl = long double;
using ull = unsigned long long;

using error_code = int64_t;

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

template <typename T> static inline auto powerof2_roundup(T x) -> T
{
    return 1ull << (sizeof(T) * 8 - __builtin_clzll(x));
}

template <typename T> static inline auto log2(T x) -> T
{
    unsigned long long val = x;
    return ((T)(8 * sizeof(unsigned long long) - __builtin_clzll((val)) - 1));
}

// round down to the nearest multiple of n
template <typename T> static inline constexpr T rounddown(T val, T n)
{
    return val - val % n;
}

// round up to the nearest multiple of n
template <typename T> static inline constexpr T roundup(T val, T n)
{
    return rounddown(val + n - 1, n);
}

#define container_of(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))

// linked list head
struct list_head
{
    list_head *next, *prev;
};

using list_foreach_func = void (*)(list_head *);

template <size_t size_in_byte> using BLOCK = uint8_t[size_in_byte];