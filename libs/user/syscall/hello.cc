//
// Created by bear on 5/31/20.
//
#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API size_t hello(size_t a, size_t b, size_t c, size_t d)
{
    return make_syscall(syscall::SYS_hello, a, b, c, d);
}