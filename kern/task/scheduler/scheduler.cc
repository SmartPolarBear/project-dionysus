#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "drivers/cmos/rtc.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace lock;
using namespace trap;

// TODO: cpu scheduler ( load balance )


void task::scheduler::reschedule()
{
	KDEBUG_ASSERT(!global_thread_lock.holding());

	lock_guard guard{ global_thread_lock };

	reschedule_locked();
}

void task::scheduler::reschedule_locked() TA_REQ(global_thread_lock)
{
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

	yield_locked();
}

void task::scheduler::yield_locked() TA_REQ(global_thread_lock)
{
	cur_thread->state = thread::thread_states::READY;

	cur_thread->need_reschedule_ = true;
}

void task::scheduler::schedule()
{
	KDEBUG_ASSERT(this->owner_cpu);

	KDEBUG_ASSERT(!global_thread_list.empty());
	KDEBUG_ASSERT(global_thread_lock.holding());

	trap::pushcli();

	thread* next = nullptr;

	cur_thread->need_reschedule_ = false;

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

	check_timers();
	tick(cur_thread.get());
}

void task::scheduler::unblock_locked(task::thread* t)
{
	t->state = thread::thread_states::READY;
	enqueue(t);
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
	insert_locked(t);
}

void task::scheduler::insert_locked(task::thread* t) TA_REQ(global_thread_lock)
{
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
	cpu_struct* max_cpu = &(*valid_cpus.begin()), * min_cpu = &(*valid_cpus.begin());
	for (auto& c:valid_cpus)
	{
		if (c.scheduler->workload_size() > max_cpu->scheduler->workload_size())
		{
			max_cpu = &c;
		}

		if (c.scheduler->workload_size() < min_cpu->scheduler->workload_size())
		{
			min_cpu = &c;
		}
	}

	if (max_cpu != min_cpu && max_cpu->scheduler->workload_size() > min_cpu->scheduler->workload_size() + 1)
	{
		if (auto victim = max_cpu->scheduler->steal();victim != nullptr)
		{
			min_cpu->scheduler->enqueue(victim);
		}
	}

	if (t != cpu->idle)
	{
		scheduler_class.tick();
	}
	else if (t != nullptr) // idle is running
	{
		t->need_reschedule_ = true;
	}

	popcli();
}

task::scheduler::size_type task::scheduler::workload_size() const
{
	return scheduler_class.workload_size();
}

error_code task::scheduler::idle(void* arg __UNUSED) TA_NO_THREAD_SAFETY_ANALYSIS
{
	for (;;)
	{
		// Pull migration approach to load balancing
		{
			auto intr = arch_ints_disabled();

			if (!intr)
			{
				cli();
			}

			lock_guard g2{ global_thread_lock };
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

			if (!intr)
			{
				sti();
			}
		}
		scheduler::current::reschedule();
	}

	// assert no return
	KDEBUG_ASSERT(ERROR_SHOULD_NOT_REACH_HERE);
}

task::thread* task::scheduler::steal()
{
	return scheduler_class.steal(cpu.get());
}

void task::scheduler::add_timer(task::scheduler_timer* timer)
{
	lock_guard g{ timer_lock };

	auto iter = timer_list.begin();
	while (iter != timer_list.end())
	{
		if (timer->expires < iter->expires)
		{
			iter->expires -= timer->expires;
			break;
		}

		timer->expires -= iter->expires;
		iter++;
	}

	if (iter != timer_list.begin())iter--;

	timer_list.insert(iter, timer);
}

void task::scheduler::remove_timer(task::scheduler_timer* timer)
{
	lock_guard g{ timer_lock };

	if (timer_list.empty())return;

	if (timer->expires != 0)
	{
		auto next = timer->link.next->parent;
		if (next != nullptr)
		{
			next->expires += timer->expires;
		}
	}

	timer_list.remove(timer);
}

void task::scheduler::check_timers()
{
	if (timer_list.empty())return;

	auto timer = timer_list.front_ptr();
	--timer->expires;

	while (!timer->expires)
	{
		auto next = timer->link.next->parent;

		timer->callback(timer, cmos::cmos_read_rtc_timestamp(), timer->arg);

		remove_timer(timer);
		if (!next) break;
		timer = next;
	}
}

void task::scheduler::current::reschedule()
{
	cpu->scheduler->reschedule();
}

void task::scheduler::current::reschedule_locked() TA_REQ(global_thread_lock)
{
	cpu->scheduler->reschedule_locked();
}

void task::scheduler::current::yield()
{
	cpu->scheduler->yield();
}

bool task::scheduler::current::unblock(task::thread* t)
{
	if (t->affinity_.cpu != CPU_NUM_INVALID)
	{
		KDEBUG_ASSERT(t->affinity_.cpu < valid_cpus.size());
		valid_cpus[t->affinity_.cpu].scheduler->unblock_locked(t);

		return t->affinity_.cpu == cpu->id;
	}

	cpu->scheduler->unblock_locked(t);
	return false;
}

void task::scheduler::current::insert(task::thread* t)
{
	if (t->affinity_.cpu != CPU_NUM_INVALID)
	{
		KDEBUG_ASSERT(t->affinity_.cpu < valid_cpus.size());
		valid_cpus[t->affinity_.cpu].scheduler->insert(t);
	}

	cpu->scheduler->insert(t);
}

void task::scheduler::current::timer_tick_handle()
{
	cpu->scheduler->timer_tick_handle();
}

void task::scheduler::current::enter()
{
	// enter scheduler by directly calling the idle routine
	cpu->scheduler->idle(nullptr);
}

void task::scheduler::current::block_locked()
{
	cpu->scheduler->reschedule_locked();
}

bool task::scheduler::current::unblock_locked(wait_queue::wait_queue_list_type threads)
{
	KDEBUG_ASSERT(global_thread_lock.holding());

	while (!threads.empty())
	{
		auto t = threads.front_ptr();
		threads.pop_front();

		KDEBUG_ASSERT(!t->is_idle());

		t->state = thread::thread_states::READY;
		scheduler::current::unblock(t);
	}

	return true;
}
