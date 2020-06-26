#pragma once

#if defined(__ASSEMBLER__)

#define KERNEL_GS_KSTACK 0
#define KERNEL_GS_USTACK 8

#else

#include "system/error.h"
#include "system/types.h"

#include "system/syscall.h"

using syscall::syscall_regs;

enum KERNEL_GS_INDEX
{
	KERNEL_GS_KSTACK = 0,
	KERNEL_GS_USTACK = 8,
	KERNEL_SYSCALL_CONTEXT_ADDR = 16,
	KERNEL_SYSCALL_CONTEXT = 24, // next info should add a sizeof(syscall_regs) to 24
};

size_t get_nth_arg(const syscall_regs* regs, size_t n);
size_t get_syscall_number(const syscall_regs* regs);

extern "C" error_code syscall_body();

extern "C" void syscall_x64_entry();

#endif
