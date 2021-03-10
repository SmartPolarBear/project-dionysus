#include "ipc.hpp"
#include "syscall_client.hpp"

extern "C" error_code ipc_load_message(task::ipc::message* msg)
{
	return make_syscall(syscall::SYS_ipc_load_message, msg);
}