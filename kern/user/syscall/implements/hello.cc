#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "builtin_text_io.hpp"

error_code sys_hello(const syscall_regs* regs)
{

	write_format("[pid %d]hello ! %lld %lld %lld %lld\n",
		cur_proc->get_id(),
		get_nth_arg(regs, 0),
		get_nth_arg(regs, 1),
		get_nth_arg(regs, 2),
		get_nth_arg(regs, 3));

	return ERROR_SUCCESS;
}