//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "system/messaging.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API error_code terminate(error_code e)
{
	return make_syscall(syscall::SYS_exit, e);
}

DIONYSUS_API error_code set_heap_size(uintptr_t* size)
{
	return make_syscall(syscall::SYS_set_heap_size, size);
}
