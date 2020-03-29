#pragma once

#include "sys/types.h"
#include "sys/error.h"

extern error_code syscall_body();

extern void syscall_entry_amd64();