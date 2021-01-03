#pragma once

#include "system/types.h"
#include "system/error.hpp"
#include "system/messaging.hpp"

extern "C" error_code terminate(error_code e);
extern "C" error_code set_heap_size(uintptr_t* size);

void heap_free(void* ap);
void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags);

extern "C" error_code ipc_send(pid_type pid, IN const void* msg, size_t size);
extern "C" error_code ipc_send_page(pid_type pid, uint64_t value, IN const void* src, size_t perm);
extern "C" error_code ipc_receive(OUT void* msg);
extern "C" error_code ipc_receive_page(OUT  void* dst, OUT uint64_t* out_val, OUT pid_type* out_pid, OUT size_t* perms);

extern "C" size_t hello(size_t a, size_t b, size_t c, size_t d);
extern "C" size_t put_str(const char* str);
extern "C" size_t put_char(size_t ch);

void write_format(const char* fmt, ...);
void write_format_a(const char* fmt, va_list ap);

// returns the length of the result string
size_t itoa_ex(char* buf, unsigned long long n, int base);

// returns the length of the result string
size_t ftoa_ex(double f, char* buf, int precision);