#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "system/messaging.hpp"

#include "dionysus_api.hpp"

#include "process.hpp"

DIONYSUS_API error_code get_current_thread(OUT object::handle_type* out)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_current_thread, out);
}