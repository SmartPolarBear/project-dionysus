#pragma once

#include "system/types.h"

// returns the length of the result string
size_t itoa_ex(char *buf, unsigned long long n, int base);

// returns the length of the result string
size_t ftoa_ex(double f, char *buf, int precision);