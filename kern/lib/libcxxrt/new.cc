#include "lib/libcxx/new"
#include "sys/types.h"

//Exceptions aren't supported in kernel, so we mark these operations noexcept
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnew-returns-null"

void operator delete(void *ptr) noexcept
{
    //TODO: Implement later : Call kernel free
    ptr = nullptr;
}

void *operator new(size_t len)
{
    //TODO: Implement later : Call kernel allocate
    return nullptr;
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

void operator delete(void *, void *)noexcept
{
    //Do nothing
}

void operator delete[](void *, void *) noexcept
{
    //Do nothing
}
