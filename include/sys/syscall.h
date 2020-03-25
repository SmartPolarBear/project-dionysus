#pragma once
#include "sys/error.h"
#include "sys/types.h"

namespace syscall
{

using syscall_entry = error_code (*)();
constexpr size_t SYSCALL_COUNT = 512;
extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1];

void system_call_init();

} // namespace syscall
