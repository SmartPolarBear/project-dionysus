#include "syscall_client.hpp"

// external functions.
void heap_free(void* ap);
void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags);

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

// We use default placement new in the std library