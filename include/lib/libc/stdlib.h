#if !defined(__INCLUDE_LIB_LIBC_STDLIB_H)
#define __INCLUDE_LIB_LIBC_STDLIB_H

#include "sys/types.h"

extern "C" void itoa(char *buf, size_t n, int base);

// returns the length of the result string
size_t itoa_ex(char *buf, unsigned long long n, int base);

#endif // __INCLUDE_LIB_LIBC_STDLIB_H
