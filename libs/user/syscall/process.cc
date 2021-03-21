//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "system/messaging.hpp"

#include "dionysus_api.hpp"

#include "process.hpp"

DIONYSUS_API error_code terminate(error_code e)
{
	return make_syscall(syscall::SYS_exit, e);
}

DIONYSUS_API error_code set_heap_size(uintptr_t* size)
{
	return make_syscall(syscall::SYS_set_heap_size, size);
}

DIONYSUS_API error_code get_current_process(OUT object::handle_type* out)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_current_process, out);

}

DIONYSUS_API error_code get_process_by_id(OUT object::handle_type* out, object::koid_type id)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_process_by_id, out, id);

}

DIONYSUS_API error_code get_process_by_name(OUT object::handle_type* out, const char* name)
{
	if (out == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (name == nullptr)
	{
		return -ERROR_INVALID;
	}

	return make_syscall(syscall::SYS_get_process_by_name, out, name);
}