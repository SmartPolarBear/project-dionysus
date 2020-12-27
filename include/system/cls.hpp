#pragma once

#include "system/concepts.hpp"
#include "system/types.h"
#include "system/segmentation.hpp"

#include "kbl/lock/spinlock.h"

static_assert(sizeof(uintptr_t) == 0x08);

enum CLS_ADDRESS : uintptr_t
{
	CLS_CPU_STRUCT_PTR = 0,
	CLS_PROC_STRUCT_PTR = 0x8
};

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"


// cpu local storage

template<typename T>
static inline T cls_get(uintptr_t n)
requires Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%fs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void cls_put(uintptr_t n, T v)
requires Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%fs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

template<typename T>
static inline T gs_get(uintptr_t n)
requires Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%gs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void gs_put(uintptr_t n, T v)
requires Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%gs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

#pragma clang diagnostic pop

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
class CLSItem
{
 private:
	lock::spinlock_struct lk;
	bool use_lock;
 public:
	CLSItem() : use_lock(true)
	{
		lock::spinlock_initialize_lock(&lk, "clslock");
	}

	CLSItem(bool _use_lock) : use_lock(_use_lock)
	{
		lock::spinlock_initialize_lock(&lk, "clslock");
	}

	void set_lock(bool _use_lock)
	{
		use_lock = _use_lock;
	}

	bool get_lock()
	{
		return use_lock;
	}

	T operator()()
	{
		return cls_get<T>(addr);
	}

	T operator->()
	{
		return cls_get<T>(addr);
	}

	void operator=(T src)
	{
		if (use_lock)
		{
			lock::spinlock_acquire(&this->lk);
		}

		cls_put(addr, src);

		if (use_lock)
		{
			lock::spinlock_release(&this->lk);
		}
	}

};

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, CLSItem<T, addr> rhs)
{
	return lhs() == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, CLSItem<T, addr> rhs)
{
	return !(lhs() == rhs());
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(T lhs, CLSItem<T, addr> rhs)
{
	return lhs == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(T lhs, CLSItem<T, addr> rhs)
{
	return !(lhs == rhs());
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, T rhs)
{
	return lhs() == rhs;
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, T rhs)
{
	return !(lhs() == rhs);
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, [[maybe_unused]] nullptr_t rhs)
{
	return lhs() == nullptr;
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, [[maybe_unused]]nullptr_t rhs)
{
	return !(lhs() == nullptr);
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==([[maybe_unused]]nullptr_t lhs, CLSItem<T, addr> rhs)
{
	return nullptr == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=([[maybe_unused]]nullptr_t lhs, CLSItem<T, addr> rhs)
{
	return !(nullptr == rhs());
}

