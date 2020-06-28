//
// Created by bear on 5/31/20.
//

#include "syscall_client.hpp"

#include "system/syscall.h"
#include "system/error.h"

extern "C" error_code app_terminate(error_code err)
{
   return trigger_syscall(syscall::SYS_exit, 1, err);
}