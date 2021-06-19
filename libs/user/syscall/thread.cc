#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "dionysus_api.hpp"

#include "handle_type.hpp"

#include "thread.hpp"

DIONYSUS_API error_code get_current_thread(OUT object::handle_type* out)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_current_thread, out);
}


DIONYSUS_API error_code get_thread_by_id(OUT object::handle_type* out, object::koid_type id)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_thread_by_id, out, id);

}

DIONYSUS_API error_code get_thread_by_name(OUT object::handle_type* out, const char* name)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (name == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_thread_by_name, out, name);
}