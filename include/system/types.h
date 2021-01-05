#pragma once

// va_list and some things
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define PANIC
#define OUT
#define IN
#define OPTIONAL
#define MUST_SUPPORT
#define OPTIONAL_SUPPORT

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


using pid_type = int64_t;
using logical_block_address = uintptr_t;
using file_id = int64_t;

using uid_type = uint64_t;
constexpr uid_type UID_ROOT = 0;

using gid_type = uint64_t;
using mode_type = uint64_t;

constexpr auto STORAGE_UNIT = 1024ULL;

static inline constexpr ldbl operator "" _KB(ldbl sz)
{
	return sz * STORAGE_UNIT;
}

static inline constexpr ull operator "" _KB(ull sz)
{
	return sz * STORAGE_UNIT;
}

static inline constexpr ldbl operator "" _MB(ldbl sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator "" _MB(ull sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator "" _GB(ldbl sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator "" _GB(ull sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ldbl operator "" _TB(ldbl sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

static inline constexpr ull operator "" _TB(ull sz)
{
	return sz * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT * STORAGE_UNIT;
}

template<typename T>
static inline auto powerof2_roundup(T x) -> T
{
	return 1ull << (sizeof(T) * 8 - __builtin_clzll(x));
}

template<typename T>
static inline auto log2(T x) -> T
{
	unsigned long long val = x;
	return ((T)(8 * sizeof(unsigned long long) - __builtin_clzll((val)) - 1));
}

// round down to the nearest multiple of n
template<typename T>
static inline constexpr T rounddown(T val, T n)
{
	return val - val % n;
}

// round up to the nearest multiple of n
template<typename T>
static inline constexpr T roundup(T val, T n)
{
	return rounddown(val + n - 1, n);
}

#define container_of(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))

// linked list head
struct list_head
{
	list_head* next, * prev;
};

using list_foreach_func = void (*)(list_head*);

template<size_t size_in_byte> using BLOCK = uint8_t[size_in_byte];

using byte_t = uint8_t;
using word_t = uint16_t;
using dword_t = uint32_t;

using timestamp_t = uint64_t;

using offset_t = uintptr_t;



