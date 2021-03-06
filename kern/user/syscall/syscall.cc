
#include <syscall/syscall.hpp>

#include "syscall.h"

#include "system/mmu.h"
#include "system/syscall.h"

#include "drivers/apic/traps.h"

#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "task/process/process.hpp"

#include "builtin_text_io.hpp"

using namespace syscall;
using namespace trap;

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

[[noreturn]] void system_call_entry_x86()
{
	KDEBUG_GENERALPANIC("Syscall in 32bit compatibility mode isn't supported.\n");
}

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

	// we do not support 32bit compatibility mode
	wrmsr(MSR_CSTAR, (uintptr_t)system_call_entry_x86);

	wrmsr(MSR_SYSCALL_MASK, EFLAG_TF | EFLAG_DF | EFLAG_IF |
		EFLAG_IOPL_MASK | EFLAG_AC | EFLAG_NT);
}
