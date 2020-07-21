#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

// take the following argument:
//  1. exit code, the value of which is error_codes
error_code sys_exit(const syscall_regs* regs)
{
	return process::process_terminate((error_code)get_nth_arg(regs, 0));
}

error_code sys_send(const syscall_regs* regs)
{
	return process::process_ipc_send((process_id)get_nth_arg(regs, 0),
		(void*)get_nth_arg(regs, 1),
		(size_t)get_nth_arg(regs, 2));
}

error_code sys_receive(const syscall_regs* regs)
{
	return process::process_ipc_receive((void*)get_nth_arg(regs, 0));
}

