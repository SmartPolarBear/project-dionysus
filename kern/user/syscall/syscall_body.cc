#include "syscall.h"
#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"
#include "kbl/lock/spinlock.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

#include "task/process/process.hpp"
#include "task/thread/thread.hpp"

using namespace syscall;
using namespace task;
//
//size_t get_nth_arg(const syscall_regs* regs, size_t n)
//{
//	switch (n)
//	{
//	case 0:
//		return regs->rdi;
//	case 1:
//		return regs->rsi;
//	case 2:
//		return regs->rdx;
//	case 3:
//		return regs->r10;
//	case 4:
//		return regs->r8;
//	case 5:
//		return regs->r9;
//	default:
//		KDEBUG_RICHPANIC("System call can have not more than 4 args.", "Syscall", false, "");
//	}
//}
//
//size_t get_syscall_number(const syscall_regs* regs)
//{
//	return regs->rax;
//}


//to be called in syscall_entry.S
extern "C" [[clang::optnone]] error_code syscall_body(const syscall_regs* regs)
{
//	size_t syscall_no = get_syscall_number(regs);  // first parameter
	size_t syscall_no = args_get<syscall::ARG_SYSCALL_NUM>(regs);

	if (syscall_no > SYSCALL_COUNT_MAX)
	{
		KDEBUG_FOLLOWPANIC("Syscall number out of range.");
	}

	if (cur_proc->get_flags() & task::PROC_EXITING)
	{
		//FIXME
		KDEBUG_NOT_IMPLEMENTED;
	}

	auto ret = syscall_table[syscall_no](regs);

	if (cur_proc->get_flags() & task::PROC_EXITING)
	{
		//FIXME
		KDEBUG_NOT_IMPLEMENTED;
	}

	return ret;
}