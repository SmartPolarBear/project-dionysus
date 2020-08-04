#include "server_syscalls.hpp"

// definitions
void* operator new(size_t, void* p) noexcept;
void* operator new[](size_t, void* p) noexcept;
void operator delete(void*, void*) noexcept;
void operator delete[](void*, void*) noexcept;

//Exceptions aren't supported in kernel, so we mark these operations noexcept
void operator delete(void* ptr) noexcept
{
	heap_free(ptr);
}

void* operator new(size_t len)
{
	return heap_alloc(len, 0);
}

void operator delete[](void* ptr) noexcept
{
	::operator delete(ptr);
}

void* operator new[](size_t len)
{
	return ::operator new(len);
}

void* operator new(size_t, void* p) noexcept
{
	return p;
}

void* operator new[](size_t, void* p) noexcept
{
	return p;
}

#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnew-returns-null"

void operator delete(void*, void*) noexcept
{
	//Do nothing
}

void operator delete[](void*, void*) noexcept
{
	//Do nothing
}
