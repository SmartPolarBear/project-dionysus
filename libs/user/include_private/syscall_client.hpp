//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/syscall.h"
#include "system/error.h"


// syscall without out parameters
// precondition:
//  1) 0<=syscall_number<SYSCALL_COUNT
//  2) 0<=para_count<7 and all parameter is uint64_t or those with the same size.
__attribute__((always_inline)) static inline error_code trigger_syscall(uint64_t syscall_number, size_t para_count, ...)
{
    uint64_t args[6] = {0};

    if (syscall_number >= syscall::SYSCALL_COUNT ||
        para_count > syscall::SYSCALL_PARAMETER_MAX)
    {
        return -ERROR_INVALID;
    }

    // copy out parameters in advance to avoid rewriting %rdx
    va_list ap;
    va_start(ap, para_count);
    for (size_t i = 0; i < para_count; i++)
    {
        args[i] = va_arg(ap, uint64_t);
    }
    va_end(ap);

    // set parameter-passing registers
    for (size_t i = 0; i < para_count; i++)
    {
        switch (i)
        {
            case 5:
            {
                asm volatile("mov %0, %%r9\n\t"
                :
                : "r" (args[5])
                : "%r9");
                break;
            }
            case 4:
                asm volatile("mov %0, %%r8\n\t"
                :
                : "a" (args[4])
                : "%r8");
                break;
            case 3:
                asm volatile("mov %0, %%r10\n\t"
                :
                : "a" (args[3])
                : "%r10");
                break;
            case 2:
                asm volatile("mov %0, %%rdx\n\t"
                :
                : "a" (args[2])
                : "%rdx");
                break;
            case 1:
                asm volatile("mov %0, %%rsi\n\t"
                :
                : "a" (args[1])
                : "%rsi");
                break;
            case 0:
                asm volatile("mov %0, %%rdi\n\t"
                :
                : "a" (args[0])
                : "%rdi");
                break;
            default:
                break;
        }
    }


    error_code ret = 0;

    // rcx and r11 are used by syscall instruction and therefore should be protected
    asm volatile ( "syscall" : "=a" (ret)
    : "a" (syscall_number)
    : "rcx", "r11", "rbx", "memory" );

    return ret;
}
