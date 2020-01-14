#if !defined(__INCLUDE_LIB_LIBC_STDLIB_H)
#define __INCLUDE_LIB_LIBC_STDLIB_H

#include "sys/types.h"

extern "C"[[deprecated("Deprecated for safety reasons")]] char *itoa(char *buf, size_t n, int base);
extern "C"[[deprecated("Deprecated for safety reasons")]] char *ftoa(double f, char *buf, int precision);

// returns the length of the result string
size_t itoa_ex(char *buf, unsigned long long n, int base);

// returns the length of the result string
size_t ftoa_ex(double f, char *buf, int precision);

#endif // __INCLUDE_LIB_LIBC_STDLIB_H
