#include "system/kmalloc.hpp"
#include "system/types.h"

using align_val_t = std::align_val_t;
using nothrow_t = std::nothrow_t;

// exceptional versions. we cannot use them safely.

[[nodiscard, deprecated("We can't handle exception right.")]]void* operator new(size_t len)
{
	return memory::kmalloc(len, 0);
}

[[nodiscard, deprecated("We can't handle exception right.")]]void* operator new[](size_t len)
{
	return ::operator new(len);
}

[[nodiscard, deprecated("We can't handle exception right.")]]void* operator new(size_t count, align_val_t al)
{
	count = roundup(count, (size_t)al);
	return ::operator new(count);
}

[[nodiscard, deprecated("We can't handle exception right.")]]void* operator new[](size_t count,
	align_val_t al)
{
	count = roundup(count, (size_t)al);
	return ::operator new(count);
}

// non-exception versions

[[nodiscard]]void* operator new(size_t count, [[maybe_unused]]const nothrow_t& tag) noexcept
{
	return memory::kmalloc(count, 0);
}

[[nodiscard]]void* operator new[](size_t count,  [[maybe_unused]]const nothrow_t& tag) noexcept
{
	return ::operator new(count, tag);
}

[[nodiscard]]void* operator new(size_t count,
	align_val_t al, const nothrow_t& tag) noexcept
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, tag);
}

[[nodiscard]]void* operator new[](size_t count,
	align_val_t al, const nothrow_t& tag) noexcept
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, tag);
}

// User-defined

[[nodiscard]]void* operator new(size_t count, size_t flags)
{
	return memory::kmalloc(count, flags);
}

[[nodiscard]]void* operator new[](size_t count, size_t flags)
{
	return ::operator new(count, flags);
}

[[nodiscard]]void* operator new(size_t count,
	align_val_t al, size_t flags)
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, flags);
}

void* operator new[](size_t count,
	align_val_t al, size_t flags)
{
	count = roundup(count, (size_t)al);
	return ::operator new(count, flags);
}

// We use default placement new in the std library

