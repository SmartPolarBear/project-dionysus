#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

task::scheduler::class_type scheduler_class{};

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

	context_switch(&cur_thread->kstack->context, cpu->scheduler_context);

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

			if (t.state == thread::thread_states::DYING)
			{
				t.finish_dying();
			}
		}
	}

}

void task::scheduler::unblock(task::thread* t) TA_REQ(global_thread_lock)
{

}

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{

}

void task::round_rubin_scheduler_class::dequeue(task::thread* thread)
{

}

task::thread* task::round_rubin_scheduler_class::pick_next()
{
	return nullptr;
}

void task::round_rubin_scheduler_class::timer_tick()
{

}
