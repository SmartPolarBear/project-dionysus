#include "system/kmalloc.hpp"
#include "system/types.h"

// definitions
void *operator new(size_t, void *p) noexcept;
void *operator new[](size_t, void *p) noexcept;
void operator delete(void *, void *) noexcept;
void operator delete[](void *, void *) noexcept;

//Exceptions aren't supported in kernel, so we mark these operations noexcept
void operator delete(void *ptr) noexcept
{
    memory::kfree(ptr);
}

void *operator new(size_t len)
{
    return memory::kmalloc(len, 0);
}

void operator delete[](void *ptr) noexcept
{
    ::operator delete(ptr);
}

void *operator new[](size_t len)
{
    return ::operator new(len);
}

// We use default placement new in the std library

