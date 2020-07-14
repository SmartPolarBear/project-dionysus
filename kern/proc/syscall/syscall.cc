#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

using namespace syscall;


PANIC void syscall::system_call_init()
{
	// check availability of syscall/sysret
	auto[eax, ebx, ecx, edx] = cpuid(CPUID_INTELFEATURES);

	if (!(edx & (1 << 11)))
	{
		KDEBUG_GENERALPANIC("SYSCALL/SYSRET isn't available.");
	}

	// enable the syscall/sysret instructions

	auto ia32_EFER_val = rdmsr(MSR_EFER);
	if (!(ia32_EFER_val & 0b1))
	{
		// if SCE bit is not set, set it.
		ia32_EFER_val |= 0b1;
		wrmsr(MSR_EFER, ia32_EFER_val);
	}

	wrmsr(MSR_STAR, (SEGMENTSEL_UNULL << 48ull) | (SEGMENTSEL_KCODE << 32ull));
	wrmsr(MSR_LSTAR, (uintptr_t)syscall_x64_entry);

	using namespace trap;
	wrmsr(MSR_SYSCALL_MASK, EFLAG_TF | EFLAG_DF | EFLAG_IF |
		EFLAG_IOPL_MASK | EFLAG_AC | EFLAG_NT);

	// TODO:support 32bit compatibility mode
	// we do not support 32bit compatibility mode now
	//// wrmsr(MSR_CSTAR, (uintptr_t)system_call_entry_x86);
}