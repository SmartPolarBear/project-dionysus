#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

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

	cur_thread->need_reschedule = true;
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

	cur_thread->need_reschedule = false;

	if (cur_thread->state == thread::thread_states::READY)
	{
		enqueue(cur_thread.get());
	}

	auto next = pick_next();
	if (next != nullptr)
	{
		dequeue(next);
	}

	if (next == nullptr)
	{
		next = cpu->idle;
	}

	if (next != cur_thread)
	{
		next->switch_to();
	}
}

void task::scheduler::handle_timer()
{
	lock_guard g{ global_thread_lock };

	timer_tick(cur_thread.get());
}

void task::scheduler::unblock(task::thread* t) TA_REQ(global_thread_lock)
{

}

void task::scheduler::enqueue(task::thread* t)
{
	if (cur_thread != cpu->idle)
	{
		scheduler_class.enqueue(t);
	}
}

void task::scheduler::dequeue(task::thread* t)
{
	scheduler_class.dequeue(t);
}

task::thread* task::scheduler::pick_next()
{
	return scheduler_class.pick_next();
}

void task::scheduler::timer_tick(task::thread* t)
{
	if (cur_thread != cpu->idle)
	{
		scheduler_class.timer_tick();
	}
	else
	{
		cur_thread->need_reschedule = true;
	}
}
