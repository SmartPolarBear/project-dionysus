#include "ipc.hpp"
#include "syscall_client.hpp"

DIONYSUS_API error_code ipc_load_message(task::ipc::message* msg)
{
	return make_syscall(syscall::SYS_ipc_load_message, msg);
}

DIONYSUS_API error_code ipc_send(object::handle_type target, time_type time)
{
	return make_syscall(syscall::SYS_ipc_send, target, time);
}

DIONYSUS_API error_code ipc_receive(object::handle_type from, time_type time)
{
	return make_syscall(syscall::SYS_ipc_receive, from, time);
}
