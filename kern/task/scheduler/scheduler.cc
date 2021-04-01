#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "drivers/cmos/rtc.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "ktl/algorithm.hpp"

#include "gsl/util"

using namespace lock;
using namespace trap;

// TODO: cpu scheduler ( load balance )


// Helpers to interact with scheduler class

void task::scheduler::enqueue(task::thread* t)
{
	if (t->state == thread::thread_states::DYING && t == cpu->idle)
	{
		KDEBUG_GENERALPANIC("idle thread exiting.\n");
	}
	else
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

task::thread* task::scheduler::steal()
{
	return scheduler_class.steal(cpu.get());
}

// Scheduler timer implementation

void task::scheduler::check_timers_locked()
{
	if (timer_list.empty())return;

	auto timer = timer_list.front_ptr();
	--timer->expires;

	while (!timer->expires)
	{
		auto next = timer->link.next_->parent_;

		timer->callback(timer, cmos::cmos_read_rtc_timestamp(), timer->arg);

		remove_timer(timer);
		if (!next) break;
		timer = next;
	}
}

void task::scheduler::timer_tick_handle()
{
	timer_lock.assert_not_held();

	{
		lock_guard g2{ timer_lock };
		check_timers_locked();
	}

	tick(cur_thread.get());
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
		auto next = timer->link.next_->parent_;
		if (next != nullptr)
		{
			next->expires += timer->expires;
		}
	}

	timer_list.remove(timer);
}


// Scheduler itself's implementation

void task::scheduler::insert(task::thread* t)
{
	lock_guard g{ global_thread_lock };
	insert_locked(t);
}

void task::scheduler::tick(task::thread* t)
{
	auto state = arch_interrupt_save();
	auto _ = gsl::finally([&state]()
	{
	  arch_interrupt_restore(state);
	});

	if (t != cpu->idle)
	{
		lock_guard g{ global_thread_lock };
		scheduler_class.tick();
	}
	else if (t != nullptr) // idle is running
	{
		t->need_reschedule_ = true;
	}

	// Push migrating approach to load balancing
	cpu_struct* max_cpu = &(*valid_cpus.begin()), * min_cpu = &(*valid_cpus.begin());

	{
		lock_guard g{ global_thread_lock };

		for (auto& c:valid_cpus)
		{
			if (c.scheduler->workload_size_locked() > max_cpu->scheduler->workload_size_locked())
			{
				max_cpu = &c;
			}

			if (c.scheduler->workload_size_locked() < min_cpu->scheduler->workload_size_locked())
			{
				min_cpu = &c;
			}
		}
	}

	if (max_cpu != min_cpu
		&& max_cpu->scheduler->workload_size() > min_cpu->scheduler->workload_size() + 1)
	{
		lock_guard g{ global_thread_lock };
		if (auto victim = max_cpu->scheduler->steal();victim != nullptr)
		{
			min_cpu->scheduler->enqueue(victim);
		}
	}

}

void task::scheduler::reschedule()
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);
	KDEBUG_ASSERT(!global_thread_lock.holding());

	lock_guard guard{ global_thread_lock };

	reschedule_locked();
}

void task::scheduler::reschedule_locked()
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);

	global_thread_lock.assert_held();

	// terminate current thread
	if (cur_thread->state == thread::thread_states::RUNNING)
	{
		cur_thread->state = thread::thread_states::READY;
	}

	this->schedule();
}

void task::scheduler::yield()
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);

	lock_guard g{ global_thread_lock };

	yield_locked();
}

void task::scheduler::yield_locked() TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);

	cur_thread->need_reschedule_ = true;
}

void task::scheduler::unblock_locked(task::thread* t)
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);

	t->state = thread::thread_states::READY;
	enqueue(t);
}

void task::scheduler::insert_locked(task::thread* t) TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(cpu->id == owner_cpu->id);

	enqueue(t);
}

task::scheduler::size_type task::scheduler::workload_size() const
{
	lock_guard g{ global_thread_lock };
	return workload_size_locked();
}

task::scheduler::size_type task::scheduler::workload_size_locked() const
{
	return scheduler_class.workload_size();
}

error_code task::scheduler::idle(void* arg __UNUSED) TA_NO_THREAD_SAFETY_ANALYSIS
{
	for (;;)
	{
		auto this_cpu = cpu.get();

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
				if (c.scheduler->workload_size_locked() > max_cpu->scheduler->workload_size_locked() &&
					this_cpu != &c)
				{
					max_cpu = &c;
				}
			}

			if (auto t = max_cpu->scheduler->steal();t != nullptr)
			{
				this_cpu->scheduler->enqueue(t);
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
	global_thread_lock.assert_not_held();

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

void task::scheduler::schedule()
{
	KDEBUG_ASSERT((uintptr_t)this->owner_cpu != INVALID_PTR_MAGIC);

	KDEBUG_ASSERT(!global_thread_list.empty());

	KDEBUG_ASSERT(global_thread_lock.holding());

	auto state = arch_interrupt_save();

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

	if (next != cur_thread.get())
	{
		next->switch_to(state);
	}

}