#pragma once

#if defined(__ASSEMBLER__)

#define KERNEL_GS_KSTACK 0
#define KERNEL_GS_USTACK 8

#else

#include "system/error.h"
#include "system/types.h"

enum KERNEL_GS_INDEX
{
    KERNEL_GS_KSTACK = 0,
    KERNEL_GS_USTACK = 8,
};

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

extern "C" error_code syscall_body();

extern "C" void syscall_x64_entry();

#endif
