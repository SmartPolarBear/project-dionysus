#pragma once

#include "system/types.h"

void write_format(const char *fmt, ...);
void valist_write_format(const char *fmt, va_list args);
size_t valist_write_format(char *buf, size_t n, const char *fmt, va_list ap);
void put_char(char c);
void put_str(const char *str);