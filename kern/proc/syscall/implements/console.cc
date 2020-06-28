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

error_code sys_putstr(const syscall_regs* regs)
{
	char* strbuf = (char*)get_nth_arg(regs, 0);
	put_str(strbuf);

	return ERROR_SUCCESS;
}