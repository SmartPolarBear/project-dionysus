#include "arch/amd64/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "system/error.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "libraries/libkernel/console/builtin_console.hpp"
#include <cstring>

// only works after gdt installation
void trap::pushcli(void)
{
	auto eflags = read_eflags();

	cli();
#ifndef USE_NEW_CPU_INTERFACE
	if (cpu->nest_pushcli_depth++ == 0)
	{
		cpu->intr_enable = eflags & EFLAG_IF;
	}
#else
	KDEBUG_ASSERT(cpu() != nullptr);
	if (cpu()->nest_pushcli_depth++ == 0)
	{
		cpu()->intr_enable = eflags & EFLAG_IF;
	}
#endif
}

// only works after gdt installation

void trap::popcli(void)
{
	if (read_eflags() & EFLAG_IF)
	{
		KDEBUG_RICHPANIC("Can't be called if interrupts are enabled",
			"KERNEL PANIC: SPINLOCK",
			false,
			"");
	}

#ifndef USE_NEW_CPU_INTERFACE
	--cpu->nest_pushcli_depth;
#else
	--cpu()->nest_pushcli_depth;

#endif

#ifndef USE_NEW_CPU_INTERFACE
	KDEBUG_ASSERT(cpu->nest_pushcli_depth >= 0);
	if (cpu->nest_pushcli_depth == 0 && cpu->intr_enable)
	{
		sti();
	}
#else
	KDEBUG_ASSERT(cpu() != nullptr);
	KDEBUG_ASSERT(cpu()->nest_pushcli_depth >= 0);
	if (cpu()->nest_pushcli_depth == 0 && cpu()->intr_enable)
	{
		sti();
	}
#endif
}
