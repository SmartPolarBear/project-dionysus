//
// Created by bear on 6/28/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

extern "C" size_t put_str(const char* str)
{
	return trigger_syscall(syscall::SYS_put_str, 1, (uintptr_t)str);
}

extern "C" size_t put_char(size_t ch)
{
	return trigger_syscall(syscall::SYS_put_char, 1, (uintptr_t)ch);
}