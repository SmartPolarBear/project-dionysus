#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;
using namespace trap;

void task::scheduler::reschedule()
{
	KDEBUG_ASSERT(!global_thread_lock.holding());

	ktl::mutex::lock_guard guard{ global_thread_lock };

	schedule();

	__UNREACHABLE;
}

void task::scheduler::yield()
{
	lock_guard g{ global_thread_lock };

	cur_thread->state = thread::thread_states::READY;

	cur_thread->need_reschedule = true;
}

void task::scheduler::schedule()
{
	KDEBUG_ASSERT(!global_thread_list.empty());

	thread* next = nullptr;

	cur_thread->need_reschedule = false;
	if (cur_thread->state == thread::thread_states::READY)
	{
		enqueue(cur_thread.get());
	}

	next = pick_next();
	if (next != nullptr)
	{
		dequeue(next);
	}

	if (next == nullptr)
	{
		next = cpu->idle;
	}

	if (next != cur_thread.get())
	{
		next->switch_to();
	}

}

void task::scheduler::handle_timer_tick()
{
	lock_guard g{ global_thread_lock };

	timer_tick(cur_thread.get());
}

void task::scheduler::unblock(task::thread* t) TA_REQ(global_thread_lock)
{
	// TODO: cpu affinity

	t->state = thread::thread_states::READY;
	cpu->scheduler.insert(t);
}

void task::scheduler::enqueue(task::thread* t)
{
	if (t != cpu->idle)
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
	pushcli();

	{
		lock_guard g{ this->timer_lock };
		if (!timer_list.empty())
		{
			// TODO
		}
	}

	if (t != cpu->idle)
	{
		scheduler_class.timer_tick();
	}
	else
	{
		t->need_reschedule = true;
	}

	popcli();
}

void task::scheduler::insert(task::thread* t) TA_REQ(global_thread_lock)
{
	enqueue(t);
}
