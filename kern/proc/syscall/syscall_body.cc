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


extern "C" error_code syscall_body()
{
    // saved registers is right in the stack
    syscall_regs *uregs = gs_get<syscall_regs *>(KERNEL_GS_KSTACK);


    size_t syscall_no = uregs->rdi; // first parameter

    switch (syscall_no)
    {
        default:
            kdebug::kdebug_warning("The syscall %lld isn't yet defined.", syscall_no);
            break;
    }

    return ERROR_SUCCESS;
}