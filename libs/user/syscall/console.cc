//
// Created by bear on 6/28/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API size_t put_str(const char* str)
{
	return make_syscall(syscall::SYS_put_str, (uintptr_t)str);
}

DIONYSUS_API size_t put_char(size_t ch)
{
	return make_syscall(syscall::SYS_put_char, (uintptr_t)ch);
}