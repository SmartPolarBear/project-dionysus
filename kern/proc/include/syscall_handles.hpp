//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.h"

#include "system/syscall.h"

error_code default_syscall(const syscall_regs *regs);
error_code sys_hello(const syscall_regs *regs);
error_code sys_exit(const syscall_regs *regs);
