//
// Created by bear on 5/31/20.
//
#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

extern "C" size_t hello(size_t a, size_t b, size_t c, size_t d)
{
    return trigger_syscall(syscall::SYS_hello, 4, a, b, c, d);
}