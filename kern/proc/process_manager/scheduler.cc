#include "process.hpp"

#include "arch/amd64/atomic.h"
#include "arch/amd64/cpu.h"

#include "system/error.h"
#include "system/memlayout.h"
#include "system/proc.h"
#include "system/scheduler.h"
#include "system/types.h"
#include "system/vmm.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "libraries/libkern/data/list.h"

using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;
using lock::spinlock_holding;

[[noreturn]] [[clang::optnone]] void scheduler::scheduler_thread()
{
	for (;;)
	{
		// enable interrupt
		sti();

		spinlock_acquire(&proc_list.lock);

		// find next runnable one
		process::process_dispatcher* runnable = nullptr;
		list_head* iter = nullptr;
		list_for(iter, &proc_list.run_head)
		{
			auto iter_proc = list_entry(iter, process::process_dispatcher, link);
			if (iter_proc->state == process::PROC_STATE_RUNNABLE)
			{
				runnable = iter_proc;
				break;
			}
		}

		if (runnable == nullptr)
		{
			spinlock_release(&proc_list.lock);
			hlt();
		}

		memmove(runnable->tf, &runnable->trapframe, sizeof(*runnable->tf)); // TODO
		process::process_run(runnable);

		spinlock_release(&proc_list.lock);

		current = nullptr;
	}
}

[[clang::optnone]] void scheduler::scheduler_enter()
{
	if (!spinlock_holding(&proc_list.lock))
	{
		KDEBUG_GENERALPANIC("scheduler_yield should hold proc_list.lock");
	}

	if (cpu->nest_pushcli_depth != 1)
	{
		KDEBUG_GENERALPANIC("scheduler_yield should hold proc_list.lock");
	}

	if (current->state == process::PROC_STATE_RUNNING)
	{
		KDEBUG_GENERALPANIC("scheduler_yield should have current proc not running");
	}

	if (read_eflags() & trap::EFLAG_IF)
	{
		KDEBUG_GENERALPANIC("scheduler_yield can't be interruptible");
	}

	auto intr_enable = cpu->intr_enable;
	context_switch(&current->context, cpu->scheduler);
	cpu->intr_enable = intr_enable;
}

[[clang::optnone]] void scheduler::scheduler_yield()
{
	spinlock_acquire(&proc_list.lock);
	current->state = process::PROC_STATE_RUNNABLE;
	scheduler_enter();
	spinlock_release(&proc_list.lock);
}

[[clang::optnone]] void scheduler::scheduler_halt()
{
	bool has_proc = false;

	list_head* iter = nullptr;
	list_for(iter, &proc_list.run_head)
	{
		auto entry = list_entry(iter, process::process_dispatcher, link);
		if (entry->state == process::PROC_STATE_RUNNING ||
			entry->state == process::PROC_STATE_RUNNABLE ||
			entry->state == process::PROC_STATE_ZOMBIE ||
			entry->state == process::PROC_STATE_SLEEPING)
		{
			has_proc = true;
			break;
		}
	}

	current = nullptr;
	vmm::install_kernel_pml4t();

	spinlock_release(&proc_list.lock);

	asm volatile(
	"movq $0, %%rbp\n"
	"movq %0, %%rsp\n"
	"pushq $0\n"
	"pushq $0\n"
	"hlt\n"
	:
	: "a"(cpu->tss.rsp0 /*(vmm::tss_get_rsp(cpu->get_tss(), 0)*/));
}
