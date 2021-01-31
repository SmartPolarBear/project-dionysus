#pragma once
#include "system/kmalloc.hpp"
#include "system/types.h"

#include "debug/kdebug.h"

#include "compiler/compiler_extensions.hpp"

namespace kbl
{
class allocate_checker final
{
 public:
	allocate_checker() = default;

	~allocate_checker()
	{
		if (armed) [[unlikely]]
		{
			panic_unchecked();
		}
	}

	void arm(size_t size, bool res)
	{
		if (armed) [[unlikely]]
		{
			panic_doubly_armed();
		}

		armed = true;
		ok = (size == 0 || res);
	}

	bool check()
	{
		armed = false;
		return ok;
	}

 private:
	bool armed = false;
	bool ok = false;

	[[noreturn]] static void panic_unchecked()
	{
		KDEBUG_GENERALPANIC("allocate_checker::check() isn't called");
	}

	[[noreturn]] static void panic_doubly_armed()
	{
		KDEBUG_GENERALPANIC("allocate_checker::arm() is called twice");
	}
};
}

[[nodiscard]] inline void* operator new(size_t count, size_t flags, kbl::allocate_checker* ck) noexcept
{
	auto ret = memory::kmalloc(count, flags);
	ck->arm(count, ret != nullptr);
	return ret;
}

[[nodiscard]] inline void* operator new(size_t count, kbl::allocate_checker* ck) noexcept
{
	return ::operator new(count, 0, ck);
}

[[nodiscard]] inline void* operator new[](size_t count, size_t flags, kbl::allocate_checker* ck) noexcept
{
	return ::operator new(count, flags, ck);
}

[[nodiscard]] inline void* operator new(size_t count,
	std::align_val_t al, size_t flags, kbl::allocate_checker* ck) noexcept
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, flags, ck);
}

[[nodiscard]] inline void* operator new[](size_t count,
	std::align_val_t al, size_t flags, kbl::allocate_checker* ck) noexcept
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, flags, ck);
}
