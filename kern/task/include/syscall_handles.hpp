//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "system/syscall.h"

#ifdef DEF_SYSCALL_HANDLE
#error "DEF_SYSCALL_HANDLE should only be defined once in this file."
#endif

#define DEF_SYSCALL_HANDLE(handle_name) error_code handle_name(const syscall::syscall_regs *regs)

DEF_SYSCALL_HANDLE(zero_is_invalid_syscall);
DEF_SYSCALL_HANDLE(default_syscall);

DEF_SYSCALL_HANDLE(sys_hello);
DEF_SYSCALL_HANDLE(sys_put_str);
DEF_SYSCALL_HANDLE(sys_put_char);
DEF_SYSCALL_HANDLE(sys_exit);
DEF_SYSCALL_HANDLE(sys_set_heap);

// task/ipc/syscall/ipc.cc
DEF_SYSCALL_HANDLE(sys_ipc_load_message);
DEF_SYSCALL_HANDLE(sys_ipc_send);
DEF_SYSCALL_HANDLE(sys_ipc_receive);

#undef DEF_SYSCALL_HANDLE