//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

#include "system/messaging.hpp"

extern "C" error_code terminate(error_code e)
{
	return trigger_syscall(syscall::SYS_exit, 1, e);
}

extern "C" error_code set_heap(uintptr_t* size)
{
	return trigger_syscall(syscall::SYS_set_heap_size, 1, size);
}

extern "C" error_code ipc_send(process_id pid, IN const void* msg, size_t size)
{
	return trigger_syscall(syscall::SYS_send, 3, pid, msg, size);
}

extern "C" error_code ipc_send_page(process_id pid, uint64_t value, IN const void* src, size_t perm)
{
	return trigger_syscall(syscall::SYS_send, 4, pid, value, src, perm);
}

extern "C" error_code ipc_receive(OUT  void* msg)
{
	return trigger_syscall(syscall::SYS_receive, 1, msg);
}

extern "C" error_code ipc_receive_page(OUT  void* dst, OUT process_id* out_pid, OUT size_t* perms)
{
	return trigger_syscall(syscall::SYS_receive, 3, dst, out_pid, perms);
}

