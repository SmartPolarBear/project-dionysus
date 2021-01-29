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

void task::scheduler::handle_timer_tick()
{
	lock_guard g1{ global_thread_lock };

	tick(cur_thread.get());
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

task::thread* task::scheduler::fetch()
{
	return scheduler_class.fetch();
}

void task::scheduler::insert(task::thread* t)
{
	enqueue(t);
}

void task::scheduler::tick(task::thread* t)
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
	return scheduler_class.workload_size();
}

error_code task::scheduler::idle(void* arg)
{
	for (;;)
	{
		auto intr = arch_ints_disabled();

		if (!intr)
		{
			cli();
		}

		{
			lock_guard g{ global_thread_lock };

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
				cpu->scheduler->enqueue(t);
			}
		}

		if (!intr)
		{
			sti();
		}

		cpu->scheduler->yield();

	}

	__UNREACHABLE; // do not do return -ERROR_SHOULD_NOT_REACH_HERE;
}

task::thread* task::scheduler::steal() TA_REQ(global_thread_lock)
{
	return scheduler_class.steal();
}

