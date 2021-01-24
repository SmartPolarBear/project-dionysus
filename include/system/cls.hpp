#pragma once

#include "ktl/concepts.hpp"
#include "system/types.h"

#include "kbl/lock/spinlock.h"

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
requires ktl::Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%fs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void cls_put(uintptr_t n, T v)
requires ktl::Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%fs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

template<typename T>
static inline T gs_get(uintptr_t n)
requires ktl::Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%gs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void gs_put(uintptr_t n, T v)
requires ktl::Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%gs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

#pragma clang diagnostic pop

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
class cls_item
{
 private:
	lock::spinlock_struct lk{};
	bool use_lock{ true };
	bool valid{ false };
 public:
	[[nodiscard]] cls_item()
	{
		lock::spinlock_initialize_lock(&lk, "clslock");
	}

	[[nodiscard]]explicit cls_item(bool _use_lock) : use_lock(_use_lock)
	{
		lock::spinlock_initialize_lock(&lk, "clslock");
	}

	[[nodiscard]]bool get_valid() const
	{
		return valid;
	}

	void set_use_lock(bool _use_lock)
	{
		use_lock = _use_lock;
	}

	[[nodiscard]]bool get_use_lock() const
	{
		return use_lock;
	}

	T get()
	{
		if (!valid)return nullptr;
		return cls_get<T>(addr);
	}

	T operator->()
	{
		if (!valid)return nullptr;
		return cls_get<T>(addr);
	}

	void operator=(T src)
	{
		if (use_lock)
		{
			lock::spinlock_acquire(&this->lk);
		}

		valid = true;
		cls_put(addr, src);

		if (use_lock)
		{
			lock::spinlock_release(&this->lk);
		}
	}

};

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator==(cls_item<T, addr> lhs, cls_item<T, addr> rhs)
{
	return lhs.get() == rhs.get();
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator!=(cls_item<T, addr> lhs, cls_item<T, addr> rhs)
{
	return !(lhs.get() == rhs.get());
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator==(T lhs, cls_item<T, addr> rhs)
{
	return lhs == rhs.get();
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator!=(T lhs, cls_item<T, addr> rhs)
{
	return !(lhs == rhs.get());
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator==(cls_item<T, addr> lhs, T rhs)
{
	return lhs.get() == rhs;
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator!=(cls_item<T, addr> lhs, T rhs)
{
	return !(lhs.get() == rhs);
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator==(cls_item<T, addr> lhs, [[maybe_unused]] nullptr_t rhs)
{
	return lhs.get() == nullptr;
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator!=(cls_item<T, addr> lhs, [[maybe_unused]]nullptr_t rhs)
{
	return !(lhs.get() == nullptr);
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator==([[maybe_unused]]nullptr_t lhs, cls_item<T, addr> rhs)
{
	return nullptr == rhs.get();
}

template<typename T, CLS_ADDRESS addr>
requires ktl::Pointer<T>
static inline bool operator!=([[maybe_unused]]nullptr_t lhs, cls_item<T, addr> rhs)
{
	return !(nullptr == rhs.get());
}



