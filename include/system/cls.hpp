#pragma once

#include "system/concepts.hpp"
#include "system/types.h"
#include "system/segmentation.hpp"

#include "drivers/lock/spinlock.h"

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
class CLSItem
{
 private:
	lock::spinlock lk;
	bool use_lock;
 public:
	CLSItem() : use_lock(true)
	{
		lock::spinlock_initlock(&lk, "clslock");
	}

	CLSItem(bool _use_lock) : use_lock(_use_lock)
	{
		lock::spinlock_initlock(&lk, "clslock");
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

