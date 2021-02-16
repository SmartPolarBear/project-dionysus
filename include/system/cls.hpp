#pragma once

#include "arch/amd64/cpu/cls.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/concepts.hpp"



template<typename T, CLS_ADDRESS Address>
requires ktl::is_pointer<T> || ktl::is_trivially_copyable<T>
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
			if constexpr (ktl::is_pointer<T>)
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
			lock::lock_guard g{ lock };
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
		if constexpr (ktl::is_pointer<T>)
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
		if constexpr (ktl::is_pointer<T>)
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

