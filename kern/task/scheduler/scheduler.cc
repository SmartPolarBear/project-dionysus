#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

void task::scheduler::reschedule()
{
	if (!global_thread_lock.holding())
	{
		KDEBUG_GENERALPANIC("scheduler_enter should hold proc_list.lock");
	}

	if (cpu->nest_pushcli_depth != 1)
	{
		KDEBUG_GENERALPANIC("scheduler_enter should validly hold proc_list.lock");
	}

	if (cur_thread->state == thread::thread_states::RUNNING)
	{
		KDEBUG_GENERALPANIC("scheduler_enter should have current task not running");
	}

	if (read_eflags() & trap::EFLAG_IF)
	{
		KDEBUG_GENERALPANIC("scheduler_enter can't be interruptible");
	}

	auto intr_enable = cpu->intr_enable;

	context_switch(&cur_thread->kstack->context, cpu->scheduler);

	cpu->intr_enable = intr_enable;
}

void task::scheduler::yield()
{
	ktl::mutex::lock_guard g{ global_thread_lock };

	cur_thread->state = thread::thread_states::READY;

	reschedule();
}

void task::scheduler::schedule()
{
	KDEBUG_ASSERT(!global_thread_list.empty());

	ktl::mutex::lock_guard guard{ global_thread_lock };

	while (true)
	{
		for (auto& t:global_thread_list)
		{
			if (t.state == thread::thread_states::READY)
			{
				t.switch_to();
			}

			int a = 0;

			if (t.state == thread::thread_states::DYING)
			{
				t.finish_dying();
			}
		}
	}

}
