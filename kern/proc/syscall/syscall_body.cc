#include "syscall.h"

#include "system/mmu.h"
#include "system/proc.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "user-include/syscall.hpp"

#include "libraries/libkernel/console/builtin_console.hpp"

using namespace syscall;

// According to System V ABI
// Parameters are in rdi, rsi, rdx, rcx, r8, r9, stack (right to left)
// Return values are in rax and rdx

size_t get_nth_arg(const syscall_regs *uregs, size_t n)
{
    switch (n)
    {
        case 0:
            return uregs->rdi;
        case 1:
            return uregs->rsi;
        case 2:
            return uregs->rdx;
        case 3:
            return uregs->rcx;
        case 4:
            return uregs->r8;
        case 5:
            return uregs->r9;
        default:
            KDEBUG_RICHPANIC("System call can have not more than 4 args.", "Syscall", false, "");
    }
}

error_code default_syscall(const syscall_regs *regs)
{
    size_t syscall_no = get_nth_arg(regs, 0); // first parameter

    kdebug::kdebug_warning("The syscall %lld isn't yet defined.", syscall_no);

    return ERROR_SUCCESS;
}


error_code sys_hello(const syscall_regs *regs)
{

    write_format("hello ! %lld %lld %lld %lld\n",
                 get_nth_arg(regs, 1),
                 get_nth_arg(regs, 2),
                 get_nth_arg(regs, 3),
                 get_nth_arg(regs, 4));

    return ERROR_SUCCESS;
}


extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1] = {
        [0 ... SYSCALL_COUNT] = default_syscall,


        [SYS_HELLO] = sys_hello,
};



extern "C" error_code syscall_body()
{
    // saved registers is right in the stack
    const syscall_regs *uregs = gs_get<syscall_regs *>(KERNEL_GS_KSTACK);

    size_t syscall_no = get_nth_arg(uregs, 0); // first parameter

    if (syscall_no > SYSCALL_COUNT)
    {
        KDEBUG_FOLLOWPANIC("Syscall number out of range.");
    }

    auto ret = syscall_table[syscall_no](uregs);

    return ret;
}