#include "sys/kmalloc.h"
#include "sys/types.h"

// #include "lib/libcxx/new"

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

void *operator new(size_t, void *p) noexcept
{
    return p;
}

void *operator new[](size_t, void *p) noexcept
{
    return p;
}

#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnew-returns-null"

void operator delete(void *, void *)noexcept
{
    //Do nothing
}

void operator delete[](void *, void *) noexcept
{
    //Do nothing
}
