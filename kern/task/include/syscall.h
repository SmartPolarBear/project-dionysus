#pragma once

#if defined(__ASSEMBLER__) // macro definitions for assembler

#define KERNEL_GS_KSTACK 0
#define KERNEL_GS_USTACK 8

#elif defined(_DIONYSUS_KERNEL_) // definitions for kernel soruce

#include "system/error.hpp"
#include "system/types.h"

#include "system/syscall.h"

using syscall::syscall_regs;

// FIXME: put this elsewhere
enum KERNEL_GS_INDEX
{
	KERNEL_GS_KSTACK = 0,
	KERNEL_GS_USTACK = 8,
	KERNEL_SYSCALL_CONTEXT_ADDR = 16,
	KERNEL_SYSCALL_CONTEXT = 24, // next info should add a sizeof(syscall_regs) to 24
};

extern "C" void syscall_x64_entry();

#endif
