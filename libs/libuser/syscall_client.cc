//
// Created by bear on 5/30/20.
//

#include "system/syscall.h"

// syscall without out parameters
// precondition:
//  1) 0<=syscall_number<SYSCALL_COUNT
//  2) 0<=para_count<7 and all parameter is uint64_t or those with the same size.
static inline uint64_t trigger_syscall(uint64_t syscall_number, size_t para_count, ...)
{
    if (syscall_number >= syscall::SYSCALL_COUNT ||
        para_count > syscall::SYSCALL_PARAMETER_MAX)
    {
        //TODO: error
    }

    va_list ap;
    va_start(ap, para_count);

    register uint64_t arg0 asm("rdi");
    register uint64_t arg1 asm("rsi");
    register uint64_t arg2 asm("rdx");
    register uint64_t arg3 asm("r10");
    register uint64_t arg4 asm("r8");
    register uint64_t arg5 asm("r9");

    for (int i = 0; i < para_count; i++)
    {
        switch (i)
        {
            case 5:
                arg5 = va_arg(ap, uint64_t);
                break;
            case 4:
                arg4 = va_arg(ap, uint64_t);
                break;
            case 3:
                arg3 = va_arg(ap, uint64_t);
                break;
            case 2:
                arg2 = va_arg(ap, uint64_t);
                break;
            case 1:
                arg1 = va_arg(ap, uint64_t);
                break;
            case 0:
                arg0 = va_arg(ap, uint64_t);
                break;
            default:
                break;
        }
    }

    va_end(ap);

    uint64_t ret = 0;

    // rcx and r11 are used by syscall instruction and therefore should be protected
    asm volatile ( "syscall" : "=a" (ret)
    : "a" (syscall_number)
    : "rcx", "r11", "rbx", "memory" );

    return ret;
}

extern "C" size_t hello(size_t a, size_t b, size_t c, size_t d)
{
    return trigger_syscall(syscall::SYS_hello, 4,
                           a, b, c, d);
}