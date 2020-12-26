#include "process.hpp"
#include "syscall.h"

#include "arch/amd64/atomic.h"
#include "arch/amd64/cpu.h"

#include "debug/kdebug.h"

#include "system/error.hpp"
#include "system/memlayout.h"
#include "system/process.h"
#include "system/scheduler.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/cls.hpp"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "ktl/mutex/lock_guard.hpp"
#include "kbl/pod_list.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

[[noreturn, clang::optnone]] void scheduler::scheduler_loop()
{
	for (;;)
	{
		// enable interrupt
		sti();

		if (proc_list.head == nullptr)
			continue;

		spinlock_acquire(&proc_list.lock);
//		ktl::mutex::lock_guard guard{ proc_list.lockable };

		// find runnable ones, we may do exiting works and therefore may remove entries, so
		// we use list_for_safe
//		list_head* iter = nullptr, * tmp = nullptr;
//		list_for_safe(iter, tmp, &proc_list.active_head)

		for (task::process_dispatcher* iter_proc = proc_list.head, * tmp =
			static_cast<task::process_dispatcher*>(iter_proc->get_next());
			 iter_proc != nullptr;
			 iter_proc = tmp, tmp = static_cast<task::process_dispatcher*>(iter_proc == nullptr ? nullptr
																								: iter_proc->get_next()))
		{
//			auto iter_proc = list_entry(iter, task::process_dispatcher, link);
			if (iter_proc->state == task::PROC_STATE_RUNNABLE)
			{

				trap::pushcli();

				cur_proc = iter_proc;

				cur_proc->state = task::PROC_STATE_RUNNING;
				cur_proc->runs++;

				KDEBUG_ASSERT(cur_proc != nullptr);
				KDEBUG_ASSERT(cur_proc->mm != nullptr);

				lcr3(V2P((uintptr_t)cur_proc->mm->pgdir));

				auto raw_kstack = cur_proc->kstack.get();
				uintptr_t kstack_addr = (uintptr_t)raw_kstack;

				cpu()->tss.rsp0 = kstack_addr + task::process_dispatcher::KERNSTACK_SIZE;

//				swap_gs();
//				gs_put(KERNEL_GS_KSTACK, (void*)cur_proc->kstack);
//				swap_gs();

				uintptr_t* kstack_gs = (uintptr_t*)(((uint8_t*)cpu->kernel_gs) + KERNEL_GS_KSTACK);
				*kstack_gs = kstack_addr;

				trap::popcli();

				context_switch(&cpu->scheduler, cur_proc->context);

				cur_proc = nullptr;
			}

			// In scheduler, we check if there's task to be killed
			while (proc_list.zombie_queue.size())
			{
				//FIXME
//				auto zombie = proc_list.zombie_queue.front();
//				proc_list.zombie_queue.pop();
//
//				list_remove(&zombie->link);
//
//				zombie->state = task::PROC_STATE_UNUSED;
//
//				delete zombie;
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

	if (cur_proc->state == task::PROC_STATE_RUNNING)
	{
		KDEBUG_GENERALPANIC("scheduler_enter should have current task not running");
	}

	if (read_eflags() & trap::EFLAG_IF)
	{
		KDEBUG_GENERALPANIC("scheduler_enter can't be interruptible");
	}

	auto intr_enable = cpu->intr_enable;

//	context_switch(&cur_proc->context, cpu->scheduler);
	context_switch(&cur_proc->context, cpu->scheduler);

	cpu->intr_enable = intr_enable;
}

void scheduler::scheduler_yield()
{
	spinlock_acquire(&proc_list.lock);

	cur_proc->state = task::PROC_STATE_RUNNABLE;

	scheduler_enter();

	spinlock_release(&proc_list.lock);
}

