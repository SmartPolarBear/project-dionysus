#pragma once

#include "system/error.hpp"
#include "system/types.h"

#include "syscall/public/syscall.hpp"

namespace syscall
{

struct syscall_regs
{
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
};

using context = syscall_regs;

using syscall_entry = error_code (*)(const syscall_regs*);

constexpr size_t SYSCALL_COUNT = 512;
constexpr size_t SYSCALL_PARAMETER_MAX = 6;

extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1];

PANIC void system_call_init();

} // namespace syscall


