#include "syscall.h"
#include "syscall/syscall_args.hpp"

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
//	char* strbuf = (char*)get_nth_arg(regs, 0);
	char* strbuf = syscall::args_get<char*, 0>(regs);//(char*)get_nth_arg(regs, 0);

//	write_format("[cpu%d,pid %d] %s", cpu->id, current->id, strbuf);
	put_str(strbuf);

	return ERROR_SUCCESS;
}

error_code sys_put_char(const syscall_regs* regs)
{
	auto c = syscall::args_get<char, 0>(regs);
	put_char(c);

	return ERROR_SUCCESS;
}