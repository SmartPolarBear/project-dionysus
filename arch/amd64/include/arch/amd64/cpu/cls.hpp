#pragma once
#include "system/types.h"

#include "ktl/concepts.hpp"

static_assert(sizeof(uintptr_t) == 0x08);

enum CLS_ADDRESS : uintptr_t
{
	CLS_CPU_STRUCT_PTR = 0,
	CLS_PROC_STRUCT_PTR = 0x8,
	CLS_CUR_THREAD_PTR = 0x16,
};

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"


// cpu local storage

template<typename T>
static inline T cls_get(uintptr_t n)
requires ktl::is_pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%fs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void cls_put(uintptr_t n, T v)
requires ktl::is_pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%fs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

template<typename T>
static inline T gs_get(uintptr_t n)
requires ktl::is_pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%gs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void gs_put(uintptr_t n, T v)
requires ktl::is_pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%gs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

#pragma clang diagnostic pop