#include "sys/syscall.h"
#include "sys/mmu.h"

#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "lib/libc/stdio.h"

using namespace syscall;


error_code default_syscall()
{
    return ERROR_NOT_IMPL;
}

error_code sys_hello()
{
    printf("hello sys!\n");
    return ERROR_SUCCESS;
}

extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1] = {
    [0 ... SYSCALL_COUNT] = default_syscall,
    [0] = sys_hello,
};


void system_call_entry()
{
    printf("system_call_entry\n");
}

void syscall::system_call_init()
{
    wrmsr(MSR_STAR, (USER_CS << 48) | (KERNEL_CS << 32));
    wrmsr(MSR_LSTAR, (uintptr_t)system_call_entry);
    wrmsr(MSR_SYSCALL_MASK, EFLAG_TF | EFLAG_DF | EFLAG_IF |
                                EFLAG_IOPL_MASK | EFLAG_AC | EFLAG_NT);
}