#include "syscall/syscall.hpp"
#include "syscall/syscall_args.hpp"

#include "debug/kdebug.h"

using namespace syscall;

uint64_t syscall::args_get(const syscall_regs* regs, int idx)
{
	if (idx == syscall::ARG_SYSCALL_NUM)
	{
		return regs->rax;
	}

	switch (idx)
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
