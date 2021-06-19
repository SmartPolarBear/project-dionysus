#pragma once

#include "system/types.h"
#include "system/error.hpp"

extern "C" error_code terminate(error_code e);
extern "C" error_code set_heap_size(uintptr_t* size);

void heap_free(void* ap);
void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags);

extern "C" error_code ipc_send(pid_type pid, IN const void* msg, size_t size);
extern "C" error_code ipc_send_page(pid_type pid, uint64_t value, IN const void* src, size_t perm);
extern "C" error_code ipc_receive(OUT void* msg);
extern "C" error_code ipc_receive_page(OUT  void* dst, OUT uint64_t* out_val, OUT pid_type* out_pid, OUT size_t* perms);
