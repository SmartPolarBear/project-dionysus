#include "arch/amd64/cpu/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "system/error.hpp"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"
#include <cstring>

// only works after gdt installation
void trap::pushcli(void)
{
	auto eflags = read_eflags();

	cli();

	if (cpu->nest_pushcli_depth++ == 0)
	{
		cpu->intr_enable = eflags & EFLAG_IF;
	}
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

	--cpu->nest_pushcli_depth;

	KDEBUG_ASSERT(cpu->nest_pushcli_depth >= 0);
	if (cpu->nest_pushcli_depth == 0 && cpu->intr_enable)
	{
		sti();
	}
}
