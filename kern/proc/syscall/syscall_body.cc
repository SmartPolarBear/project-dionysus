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

// external variable to avoid run any function in syscall_body
// which will modifies the stack and break the reg pointer
syscall_regs regs_copy;

//FIXME: this is fragile and constantly make anything into mess
extern "C" [[clang::optnone]] error_code syscall_body()
{
	// saved registers is right in the stack
	uintptr_t sp = 0;
	asm volatile ("mov %%rsp,%0":"=r"(sp));

	// REMEMBER NOT TO CALL ANY FUNCTION BEFORE COPY THE VALUE OF THIS
	const syscall_regs* regs = reinterpret_cast<syscall_regs*>(sp + sizeof(syscall_regs) + sizeof(uint64_t));
	KDEBUG_ASSERT(regs != nullptr);

	regs_copy.rax = regs->rax;
	regs_copy.rbx = regs->rbx;
	regs_copy.rcx = regs->rcx;
	regs_copy.rdx = regs->rdx;
	regs_copy.rbp = regs->rbp;
	regs_copy.rsi = regs->rsi;
	regs_copy.rdi = regs->rdi;
	regs_copy.r8 = regs->r8;
	regs_copy.r9 = regs->r9;
	regs_copy.r10 = regs->r10;
	regs_copy.r11 = regs->r11;
	regs_copy.r12 = regs->r12;
	regs_copy.r13 = regs->r13;
	regs_copy.r14 = regs->r14;
	regs_copy.r15 = regs->r15;

	size_t syscall_no = get_syscall_number(regs);  // first parameter

	if (syscall_no > SYSCALL_COUNT)
	{
		KDEBUG_FOLLOWPANIC("Syscall number out of range.");
	}

	process::process_update_context(regs_copy);

	auto ret = syscall_table[syscall_no](regs);

	return ret;
}