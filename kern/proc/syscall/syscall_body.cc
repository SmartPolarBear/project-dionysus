#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libkernel/console/builtin_text_io.hpp"

using namespace syscall;

size_t get_nth_arg(const syscall_regs* regs, size_t n)
{
	switch (n)
	{
	case 0:
		return regs->rdi;
	case 1:
		return regs->rsi;
	case 2:
		return regs->rdx;
	case 3:
		return regs->r10;
	case 4:
		return regs->r8;
	case 5:
		return regs->r9;
	default:
		KDEBUG_RICHPANIC("System call can have not more than 4 args.", "Syscall", false, "");
	}
}

size_t get_syscall_number(const syscall_regs* regs)
{
	return regs->rax;
}


//to be called in syscall_entry.S
extern "C" [[clang::optnone]] error_code syscall_body(const syscall_regs* regs)
{
	size_t syscall_no = get_syscall_number(regs);  // first parameter

	if (syscall_no > SYSCALL_COUNT)
	{
		KDEBUG_FOLLOWPANIC("Syscall number out of range.");
	}

	if (cur_proc->flags & process::PROC_EXITING)
	{
		process::process_exit(cur_proc());
	}

	auto ret = syscall_table[syscall_no](regs);

	if (cur_proc->flags & process::PROC_EXITING)
	{
		process::process_exit(cur_proc());
	}

	return ret;
}