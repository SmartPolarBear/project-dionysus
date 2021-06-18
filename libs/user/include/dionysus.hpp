#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "dionysus_api.hpp"

#include "ipc.hpp"

#include "process.hpp"

#include "thread.hpp"

DIONYSUS_API error_code terminate(error_code e);
DIONYSUS_API error_code set_heap_size(uintptr_t* size);

DIONYSUS_API void heap_free(void* ap);
DIONYSUS_API void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags);

DIONYSUS_API size_t hello(size_t a, size_t b, size_t c, size_t d);
DIONYSUS_API size_t put_str(const char* str);
DIONYSUS_API size_t put_char(size_t ch);

void write_format(const char* fmt, ...);
void write_format_a(const char* fmt, va_list ap);

// returns the length of the result string
size_t itoa_ex(char* buf, unsigned long long n, int base);

// returns the length of the result string
size_t ftoa_ex(double f, char* buf, int precision);