//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

#include "system/messaging.hpp"

extern "C" error_code app_terminate(error_code err)
{
	return trigger_syscall(syscall::SYS_exit, 1, err);
}

extern "C" error_code app_set_heap(uintptr_t* size)
{
	return trigger_syscall(syscall::SYS_set_heap, 1, size);
}

extern "C" error_code ipc_send(process_id pid, IN const void* msg, size_t size)
{
	return trigger_syscall(syscall::SYS_send, 3, pid, msg, size);
}

extern "C" error_code ipc_receive(OUT  void* msg)
{
	return trigger_syscall(syscall::SYS_receive, 1, msg);
}