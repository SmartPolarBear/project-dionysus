#pragma once

#include "sys/types.h"
#include "sys/error.h"

extern error_code syscall_body();

extern "C" void syscall_x64_entry();