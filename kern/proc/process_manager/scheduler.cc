#include "process.hpp"
#include "syscall.h"

#include "arch/amd64/atomic.h"
#include "arch/amd64/cpu.h"

#include "system/error.h"
#include "system/memlayout.h"
#include "system/process.h"
#include "system/scheduler.h"
#include "system/types.h"
#include "system/vmm.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "libraries/libkernel/data/List.h"
#include "libraries/libkernel/console/builtin_console.hpp"

using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;
using lock::spinlock_holding;

[[noreturn, clang::optnone]] void scheduler::scheduler_loop()
{
	for (;;)
	{
		// enable interrupt
		sti();

		spinlock_acquire(&proc_list.lock);

		list_head* iter = nullptr;
		// find runnable ones
		list_for(iter, &proc_list.run_head)
		{
			auto iter_proc = list_entry(iter, process::process_dispatcher, link);
			if (iter_proc->state == process::PROC_STATE_RUNNABLE)
			{

				trap::pushcli();

				current = iter_proc;

				current->state = process::PROC_STATE_RUNNING;
				current->runs++;

				// FIXME: this failed randomly
				KDEBUG_ASSERT(current != nullptr);
				KDEBUG_ASSERT(current->mm != nullptr);

				lcr3(V2P((uintptr_t)current->mm->pgdir));

				cpu()->tss.rsp0 = current->kstack + process::process_dispatcher::KERNSTACK_SIZE;

				swap_gs();
				gs_put(KERNEL_GS_KSTACK, (void*)current->kstack);
				swap_gs();

				uintptr_t* kstack_gs = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + KERNEL_GS_KSTACK);
				*kstack_gs = current->kstack;

				trap::popcli();

				context_switch(&cpu->scheduler, current->context);

				current = nullptr;
			}
		}

		spinlock_release(&proc_list.lock);

	}
}

void scheduler::scheduler_enter()
{
	if (!spinlock_holding(&proc_list.lock))
	{
		KDEBUG_GENERALPANIC("scheduler_enter should hold proc_list.lock");
	}

	if (cpu->nest_pushcli_depth != 1)
	{
		KDEBUG_GENERALPANIC("scheduler_enter should validly hold proc_list.lock");
	}

	if (current->state == process::PROC_STATE_RUNNING)
	{
		KDEBUG_GENERALPANIC("scheduler_enter should have current proc not running");
	}

	if (read_eflags() & trap::EFLAG_IF)
	{
		KDEBUG_GENERALPANIC("scheduler_enter can't be interruptible");
	}

	auto intr_enable = cpu->intr_enable;

	context_switch(&current->context, cpu->scheduler);
	cpu->intr_enable = intr_enable;
}

void scheduler::scheduler_yield()
{
	spinlock_acquire(&proc_list.lock);
	current->state = process::PROC_STATE_RUNNABLE;
	scheduler_enter();
	spinlock_release(&proc_list.lock);
}

