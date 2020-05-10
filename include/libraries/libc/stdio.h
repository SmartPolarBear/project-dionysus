#pragma once

#include "system/types.h"

void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list args);
size_t vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
void putc(char c);
void puts(const char *str);