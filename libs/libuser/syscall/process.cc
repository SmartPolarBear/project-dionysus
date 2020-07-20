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

extern "C" error_code ipc_send(IN const MessageBase* msg)
{
	return trigger_syscall(syscall::SYS_send, 1, msg);
}

extern "C" error_code ipc_receive(OUT  MessageBase* msg)
{
	return trigger_syscall(syscall::SYS_receive, 1, msg);
}