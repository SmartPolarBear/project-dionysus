#include "./syscall.hpp"

#include "sys/mmu.h"
#include "sys/proc.h"
#include "sys/syscall.h"

#include "drivers/debug/kdebug.h"

#include "arch/amd64/cpuid.h"
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

error_code syscall_body()
{
    printf("syscall_body");
    return ERROR_SUCCESS;
}

PANIC void syscall::system_call_init()
{
    // check availablity of syscall/sysret
    auto [eax, ebx, ecx, edx] = cpuid(CPUID_INTELFEATURES);

    if (!(edx & (1 << 11)))
    {
        KDEBUG_GENERALPANIC("SYSCALL/SYSRET isn't available.");
    }

    // enanble the syscall/sysret instructions

    auto ia32_EFER_val = rdmsr(MSR_EFER);
    if (!(ia32_EFER_val & 0b1))
    {
        // if SCE bit is not set, set it.
        ia32_EFER_val |= 0b1;
        wrmsr(MSR_EFER, ia32_EFER_val);
    }

    wrmsr(MSR_STAR, 0x00180008ull << 32ull);
    wrmsr(MSR_LSTAR, (uintptr_t)syscall_entry_amd64);
    // wrmsr(MSR_CSTAR, (uintptr_t)system_call_entry_x86);
    wrmsr(MSR_SYSCALL_MASK, 0x0202);
}