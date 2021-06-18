//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"


extern "C" error_code terminate(error_code e)
{
	return make_syscall(syscall::SYS_exit, e);
}

extern "C" error_code set_heap_size(uintptr_t* size)
{
	return make_syscall(syscall::SYS_set_heap_size, size);
}

