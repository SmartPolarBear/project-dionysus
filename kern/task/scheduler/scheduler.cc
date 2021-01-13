#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

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
	cur_thread->state = thread::thread_states::READY;
	reschedule();
}

void task::scheduler::schedule()
{
	KDEBUG_ASSERT(!global_thread_list.empty());

	ktl::mutex::lock_guard guard{ global_thread_lock };

	auto pick_next = [&](thread::thread_list_type::iterator_type iter)
	{
	  iter++;

	  if (iter == global_thread_list.end())
	  {
		  return global_thread_list.begin();
	  }

	  return iter;
	};

	if (cur_thread->state == thread::thread_states::DYING)
	{
		cur_thread->finish_dying();
	}

	thread::thread_list_type::iterator_type iter_head{ &cur_thread->thread_link };
	auto from = ++iter_head;
	for (auto it = from; it != iter_head; it = pick_next(it))
	{
		if (it->state == thread::thread_states::READY)
		{
			it->switch_to();
		}
	}

}
