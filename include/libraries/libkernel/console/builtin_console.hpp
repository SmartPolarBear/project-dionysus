#pragma once

#include "system/types.h"

void WriteFormat(const char *fmt, ...);
void VaListWriteFormat(const char *fmt, va_list args);
size_t VaListWriteFormat(char *buf, size_t n, const char *fmt, va_list ap);
void PutChar(char c);
void PutChar(const char *str);