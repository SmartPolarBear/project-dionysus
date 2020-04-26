#pragma once

#include "sys/types.h"

#include <stdlib.h>

// extern "C" [[deprecated("Deprecated for safety reasons")]] char *itoa(char *buf, size_t n, int base);
// extern "C" [[deprecated("Deprecated for safety reasons")]] char *ftoa(double f, char *buf, int precision);

// extern "C" void qsort(void *basep, size_t nelems, size_t size,
//                       int (*comp)(const void *, const void *));

// returns the length of the result string
size_t itoa_ex(char *buf, unsigned long long n, int base);

// returns the length of the result string
size_t ftoa_ex(double f, char *buf, int precision);