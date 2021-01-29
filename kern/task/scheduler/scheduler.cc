#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;
using namespace trap;

// TODO: cpu scheduler ( load balance )


void task::scheduler::reschedule()
{
	KDEBUG_ASSERT(!global_thread_lock.holding());

	ktl::mutex::lock_guard guard{ global_thread_lock };

	// terminate current thread
	if (cur_thread->state == thread::thread_states::RUNNING)
	{
		cur_thread->state = thread::thread_states::READY;
	}

	this->schedule();
}

void task::scheduler::yield()
{
	lock_guard g{ global_thread_lock };

	cur_thread->state = thread::thread_states::READY;

	cur_thread->need_reschedule = true;

}

void task::scheduler::schedule()
{
	KDEBUG_ASSERT(this->owner_cpu);

	KDEBUG_ASSERT(!global_thread_list.empty());
	KDEBUG_ASSERT(global_thread_lock.holding());

	trap::pushcli();

	thread* next = nullptr;

	cur_thread->need_reschedule = false;

	if (cur_thread->state == thread::thread_states::READY)
	{
		enqueue(cur_thread.get());
	}

	next = fetch();
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

void task::scheduler::timer_tick_handle()
{
	lock_guard g1{ global_thread_lock };
	lock_guard g2{ timer_lock };

	tick(cur_thread.get());
}

void task::scheduler::unblock(task::thread* t)
{
	t->state = thread::thread_states::READY;
	insert(t);
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

task::thread* task::scheduler::fetch()
{
	return scheduler_class.fetch();
}

void task::scheduler::insert(task::thread* t)
{
	lock_guard g{ global_thread_lock };

	enqueue(t);
}

void task::scheduler::tick(task::thread* t)
{
	KDEBUG_ASSERT(global_thread_lock.holding());
	pushcli();

	{

		if (!timer_list.empty())
		{
			// TODO
		}
	}

	// Push migrating approach to load balancing
//	cpu_struct* max_cpu = &(*valid_cpus.begin()), * min_cpu = &(*valid_cpus.begin());
//	for (auto& c:valid_cpus)
//	{
//		if (c.scheduler->workload_size() > max_cpu->scheduler->workload_size())
//		{
//			max_cpu = &c;
//		}
//
//		if (c.scheduler->workload_size() < min_cpu->scheduler->workload_size())
//		{
//			min_cpu = &c;
//		}
//	}
//
//	if (max_cpu != min_cpu && max_cpu->scheduler->workload_size() > min_cpu->scheduler->workload_size())
//	{
//		if (auto victim = max_cpu->scheduler->steal();victim != nullptr)
//		{
//			min_cpu->scheduler->enqueue(victim);
//		}
//	}

	if (t != cpu->idle)
	{
		scheduler_class.tick();
	}
	else if (t != nullptr)
	{
		t->need_reschedule = true;
	}

	popcli();
}

task::scheduler::size_type task::scheduler::workload_size() const
{
	lock_guard g{ global_thread_lock };
	return scheduler_class.workload_size();
}

error_code task::scheduler::idle(void* arg)
{
	kdebug::kdebug_log("idle");

	for (;;)
	{

		// Pull migration approach to load balancing
		auto intr = arch_ints_disabled();

		if (!intr)
		{
			cli();
		}

		cpu_struct* max_cpu = &valid_cpus[0];
		for (auto& c:valid_cpus)
		{
			if (c.scheduler->workload_size() > max_cpu->scheduler->workload_size() &&
				cpu.get() != &c)
			{
				max_cpu = &c;
			}
		}

		if (auto t = max_cpu->scheduler->steal();t != nullptr)
		{
			scheduler::current::insert(t);
		}

		if (!intr)
		{
			sti();
		}

		scheduler::current::reschedule();
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

task::thread* task::scheduler::steal()
{
	lock_guard g{ global_thread_lock };
	return scheduler_class.steal();
}

void task::scheduler::current::reschedule()
{
	cpu->scheduler->reschedule();
}

void task::scheduler::current::yield()
{
	cpu->scheduler->yield();
}

void task::scheduler::current::unblock(task::thread* t)
{

	if (t->affinity.cpu != CPU_NUM_INVALID)
	{
		KDEBUG_ASSERT(t->affinity.cpu < valid_cpus.size());
		valid_cpus[t->affinity.cpu].scheduler->unblock(t);
	}

	cpu->scheduler->unblock(t);
}

void task::scheduler::current::insert(task::thread* t)
{
	if (t->affinity.cpu != CPU_NUM_INVALID)
	{
		KDEBUG_ASSERT(t->affinity.cpu < valid_cpus.size());
		valid_cpus[t->affinity.cpu].scheduler->insert(t);
	}
	cpu->scheduler->insert(t);
}

void task::scheduler::current::timer_tick_handle()
{
	cpu->scheduler->timer_tick_handle();
}

void task::scheduler::current::schedule()
{
	lock_guard g{ global_thread_lock };

	cpu->scheduler->schedule();
}

task::scheduler::size_type task::scheduler::current::workload_size() TA_EXCL(global_thread_lock)
{
	return cpu->scheduler->workload_size();
}
