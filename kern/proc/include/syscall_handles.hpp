//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.h"

#include "system/syscall.h"

#define DEF_SYSCALL_HANDLE(handle_name) error_code handle_name(const syscall_regs *regs)

DEF_SYSCALL_HANDLE(default_syscall);
DEF_SYSCALL_HANDLE(sys_hello);
DEF_SYSCALL_HANDLE(sys_exit);
DEF_SYSCALL_HANDLE(sys_putstr);

#undef DEF_SYSCALL_HANDLE