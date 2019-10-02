#if !defined(__INCLUDE_LIB_LIBCXX_NEW_H)
#define __INCLUDE_LIB_LIBCXX_NEW_H
#include "sys/types.h"

//placement new
void *operator new(size_t, void *p) noexcept;
void *operator new[](size_t, void *p) noexcept;
void operator delete(void *, void *)noexcept;
void operator delete[](void *, void *) noexcept;
#endif // __INCLUDE_LIB_LIBCXX_NEW_H
