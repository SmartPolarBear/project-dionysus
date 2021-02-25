#include "syscall.h"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

error_code invalid_syscall(const syscall_regs*)
{
	KDEBUG_GENERALPANIC("syscall number 0 should never be called.");

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code default_syscall(const syscall_regs* regs)
{
	size_t syscall_no = get_syscall_number(regs); // first parameter

	kdebug::kdebug_warning("The syscall %lld isn't yet defined.", syscall_no);

	return ERROR_SUCCESS;
}