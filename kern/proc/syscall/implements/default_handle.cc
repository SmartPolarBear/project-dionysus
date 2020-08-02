#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libkernel/console/builtin_console.hpp"

error_code default_syscall(const syscall_regs *regs)
{
    size_t syscall_no = get_syscall_number(regs); // first parameter

    kdebug::kdebug_warning("The syscall %lld isn't yet defined.", syscall_no);

    return ERROR_SUCCESS;
}