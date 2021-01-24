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

template<typename T, CLS_ADDRESS Address>
requires ktl::Pointer<T> || ktl::TriviallyCopiable<T>
class cls_item
{
 public:
	[[nodiscard]] cls_item()
	{
		lock::spinlock_initialize_lock(&lk, "cls");
	}

	cls_item(const cls_item&) = delete;
	cls_item& operator=(const cls_item&) = delete;

	cls_item(cls_item&& another) noexcept
	{
		this->valid = another.valid;
		this->use_lock = another.use_lock;

		another.valid = false;
	}

	[[nodiscard]]explicit cls_item(bool _use_lock)
		: use_lock(_use_lock)
	{
		lock::spinlock_initialize_lock(&lk, "cls");
	}

	[[nodiscard]]bool is_valid() const
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

	[[nodiscard]]T get() const
	{
		if (!valid)
		{
			if constexpr (ktl::Pointer<T>)
			{
				return nullptr;
			}
			else
			{
				return T{};
			}
		}

		return cls_get<T>(Address);
	}

	void set(T val)
	{
		if (use_lock)
		{
			lock::spinlock_acquire(&this->lk);
		}

		valid = true;
		cls_put(Address, val);

		if (use_lock)
		{
			lock::spinlock_release(&this->lk);
		}
	}

	T operator->()
	{
		return get();
	}

	cls_item& operator=(const T& src)
	{
		set(src);
		return *this;
	}

	bool operator==(const cls_item& another) const
	{
		return get() == another.get();
	}

	bool operator!=(const cls_item& another) const
	{
		return !(*this == another);
	}

	bool operator==(const T& another) const
	{
		return get() == another;
	}

	bool operator!=(const T& another) const
	{
		return get() != another;
	}

	bool operator==(nullptr_t)
	{
		if constexpr (ktl::Pointer<T>)
		{
			return get() == nullptr;
		}
		else
		{
			return false;
		}
	}

	bool operator!=(nullptr_t)
	{
		if constexpr (ktl::Pointer<T>)
		{
			return get() != nullptr;
		}
		else
		{
			return false;
		}
	}

 private:
	lock::spinlock_struct lk{};
	bool use_lock{ true };

	bool valid{ false };

};

