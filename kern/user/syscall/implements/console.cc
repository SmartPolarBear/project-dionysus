#include "syscall.h"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

error_code sys_put_str(const syscall_regs* regs)
{
	char* strbuf = (char*)get_nth_arg(regs, 0);

//	write_format("[cpu%d,pid %d] %s", cpu->id, current->id, strbuf);
	put_str(strbuf);

	return ERROR_SUCCESS;
}

error_code sys_put_char(const syscall_regs* regs)
{
	put_char((char)get_nth_arg(regs, 0));

	return ERROR_SUCCESS;
}