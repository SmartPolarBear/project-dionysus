#if !defined(__INCLUDE_LIB_LIBC_STDLIB_H)
#define __INCLUDE_LIB_LIBC_STDLIB_H

#include "sys/types.h"
extern "C"
{
    void itoa(char *buf, size_t n, int base);
}

#endif // __INCLUDE_LIB_LIBC_STDLIB_H
