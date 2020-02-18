#pragma once

#include "sys/types.h"

void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list args);
void putc(char c);
void puts(const char *str);