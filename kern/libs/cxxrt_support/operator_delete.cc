#include "system/kmalloc.hpp"
#include "system/types.h"

// Operator delete and delete[] are, by definition, noexcept and therefore needs no special attention

void operator delete(void* ptr) noexcept
{
	memory::kfree(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
	memory::kfree(ptr);
}

void operator delete[](void* ptr) noexcept
{
	::operator delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept
{
	::operator delete(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] const std::nothrow_t& tag) noexcept
{
	::operator delete(ptr);
}

void operator delete(void* ptr, std::align_val_t, [[maybe_unused]] const std::nothrow_t& tag) noexcept
{
	::operator delete(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] const std::nothrow_t& tag) noexcept
{
	::operator delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t, [[maybe_unused]] const std::nothrow_t& tag) noexcept
{
	::operator delete(ptr);
}


// User-defined


//void operator delete(void* ptr, [[maybe_unused]] size_t flags)
//{
//	memory::kfree(ptr);
//}
//
//void operator delete[](void* ptr, [[maybe_unused]] size_t flags)
//{
//	::operator delete(ptr, flags);
//}

// We use default placement delete in the std library