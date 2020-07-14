//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

extern "C" error_code app_terminate(error_code err)
{
	return trigger_syscall(syscall::SYS_exit, 1, err);
}

extern "C" error_code send(size_t pid, size_t msg_sz, void* msg)
{
	return trigger_syscall(syscall::SYS_send, 3, pid, msg_sz, msg);
}

extern "C" error_code receive(void** msg, size_t* sz)
{
	return trigger_syscall(syscall::SYS_receive, 2, msg, sz);
}