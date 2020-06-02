#include "syscall.h"

#include "system/mmu.h"
#include "system/proc.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

error_code sys_exit(const syscall_regs *regs)
{
    write_format("current process calls exit: %lld\n", get_nth_arg(regs, 0));

    process::process_terminate();
    
    return ERROR_SUCCESS;
}