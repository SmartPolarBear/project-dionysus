#pragma once


#if defined(__ASSEMBLER__)

#define SYS_hello 1
#define SYS_exit 2

#else


#include "system/error.h"
#include "system/types.h"

namespace syscall
{
    enum SYSCALL_NUMBER
    {
        // starts from 1 for the sake of debugging
        SYS_hello = 1,
        SYS_exit,
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

    using syscall_entry = error_code (*)(const syscall_regs *);

    constexpr size_t SYSCALL_COUNT = 512;
    constexpr size_t SYSCALL_PARAMETER_MAX = 6;

    extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1];

    PANIC void system_call_init();

} // namespace syscall



#endif