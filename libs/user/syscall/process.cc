//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "system/messaging.hpp"

extern "C" error_code terminate(error_code e)
{
	return trigger_syscall(syscall::SYS_exit, 1, e);
}

extern "C" error_code set_heap_size(uintptr_t* size)
{
	return trigger_syscall(syscall::SYS_set_heap_size, 1, size);
}

extern "C" error_code ipc_send(pid_type pid, IN const void* msg, size_t size)
{
	return trigger_syscall(syscall::SYS_send, 3, pid, msg, size);
}

extern "C" error_code ipc_send_page(pid_type pid, uint64_t value, IN const void* src, size_t perm)
{
	return trigger_syscall(syscall::SYS_send_page, 4, pid, value, src, perm);
}

extern "C" error_code ipc_receive(OUT  void* msg)
{
	return trigger_syscall(syscall::SYS_receive, 1, msg);
}

extern "C" error_code ipc_receive_page(OUT  void* dst, OUT uint64_t* out_val, OUT pid_type* out_pid, OUT size_t* perms)
{
	return trigger_syscall(syscall::SYS_receive_page, 4, dst, out_val, out_pid, perms);
}

