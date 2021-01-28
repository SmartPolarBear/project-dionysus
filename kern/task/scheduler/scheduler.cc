#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;
using namespace trap;

// TODO: cpu scheduler ( load balance )

// FIXME: maybe use the intrusive list to fix? another bad situation is memory overwriting

size_t task::scheduler::called_count = 0;

void task::scheduler::reschedule()
{
	KDEBUG_ASSERT(!global_thread_lock.holding());

//	ktl::mutex::lock_guard guard{ global_thread_lock };
	global_thread_lock.lock();

	if (cur_thread->state == thread::thread_states::RUNNING)
	{
		cur_thread->state = thread::thread_states::READY;
	}

	if (!this->owner_cpu)
	{
		for (size_t i = 0; cpus[i].scheduler; i++)
		{
			kdebug::kdebug_log("*cpu %d:%d\n" + (this != cpus[i].scheduler),
				i,
				cpus[i].scheduler->scheduler_class.run_queue.size());
		}

		int a = 0;
	}

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
	KDEBUG_ASSERT(global_thread_lock.holding());

	trap::pushcli();

	thread* next = nullptr;

	cur_thread->need_reschedule = false;

	if (cur_thread->state == thread::thread_states::READY)
	{
		enqueue(cur_thread.get());
	}

	if (!this->owner_cpu)
	{
		for (size_t i = 0; cpus[i].scheduler; i++)
		{
			kdebug::kdebug_log("*cpu %d:%d\n" + (this != cpus[i].scheduler),
				i,
				cpus[i].scheduler->scheduler_class.run_queue.size());
		}
		KDEBUG_GENERALPANIC("FUCK");
	}

	KDEBUG_ASSERT(this->owner_cpu);
	next = pick_next();
	if (next != nullptr)
	{
		dequeue(next);
	}

	if (next == nullptr)
	{
		next = cpu->idle;
	}

	trap::popcli();

	if (next != cur_thread.get())
	{
		next->switch_to();
	}

}

void task::scheduler::handle_timer_tick()
{
	lock_guard g1{ global_thread_lock };

	timer_tick(cur_thread.get());
}

void task::scheduler::unblock(task::thread* t)
{
	// TODO: cpu affinity

	t->state = thread::thread_states::READY;
	cpu->scheduler->insert(t);
}

void task::scheduler::enqueue(task::thread* t)
{
	if (t->state == thread::thread_states::DYING)
	{
		if (t == cpu->idle)
		{
			KDEBUG_GENERALPANIC("idle thread exiting.\n");
		}

		scheduler_class.enqueue(t);
	}
	else if (t != cpu->idle)
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

void task::scheduler::insert(task::thread* t)
{
	enqueue(t);
}

void task::scheduler::timer_tick(task::thread* t)
{
	pushcli();

	{
		lock_guard g2{ timer_lock };

		if (!timer_list.empty())
		{
			// TODO
		}
	}

	if (t != cpu->idle)
	{
		scheduler_class.timer_tick();
	}
	else if (t != nullptr)
	{
		t->need_reschedule = true;
	}

	popcli();
}
