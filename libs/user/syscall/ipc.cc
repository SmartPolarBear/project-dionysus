#include "ipc.hpp"

extern "C" error_code ipc_load_message(task::ipc::message* msg)
{
	return ERROR_SUCCESS;
}