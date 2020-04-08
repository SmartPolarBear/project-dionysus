#pragma once

#if defined(__ASSEMBLER__)
#define KERNEL_GS_KSTACK 0
#define KERNEL_GS_USTACK 8
#else

#include "sys/error.h"
#include "sys/types.h"

enum KERNEL_GS_INDEX
{
    KERNEL_GS_KSTACK = 0,
    KERNEL_GS_USTACK = 8,
};

extern "C" error_code syscall_body();

extern "C" void syscall_x64_entry();

#endif
