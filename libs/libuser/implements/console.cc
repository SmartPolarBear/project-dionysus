//
// Created by bear on 6/28/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

extern "C" size_t put_str(const char* str)
{
	return trigger_syscall(syscall::SYS_put_str, 1, (uintptr_t)str);
}