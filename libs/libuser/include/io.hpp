#pragma once

#include "system/types.h"
#include "system/error.h"

extern "C" size_t hello(size_t a, size_t b, size_t c, size_t d);
extern "C" size_t put_str(const char* str);
extern "C" size_t put_char(size_t ch);

void write_format(const char* fmt, ...);
void write_format_a(const char* fmt, va_list ap);