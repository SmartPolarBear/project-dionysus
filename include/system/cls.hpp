#pragma once

#include "system/types.h"

#include "kbl/lock/spinlock.h"

#include "ktl/mutex/lock_guard.hpp"
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
	[[nodiscard]] cls_item() = default;

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

	[[nodiscard]]T get() const TA_NO_THREAD_SAFETY_ANALYSIS
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

	void set(T val) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (use_lock)
		{
			ktl::mutex::lock_guard g{ lock };
			do_set(val);
		}
		else
		{
			do_set(val);
		}
	}

	void reset()
	{
		valid = false;
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

	cls_item& operator=(nullptr_t)
	{
		reset();
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
	void do_set(T val)
	{
		valid = true;
		cls_put(Address, val);
	}

	bool use_lock{ true };

	bool valid{ false };

	mutable lock::spinlock lock{ "cls" };

};

