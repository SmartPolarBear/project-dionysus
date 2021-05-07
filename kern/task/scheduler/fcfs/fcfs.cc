#include "internals/thread.hpp"

#include "task/scheduler/fcfs/fcfs.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "drivers/acpi/cpu.h"

#include "system/scheduler.h"

#include "kbl/data/utility.hpp"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/algorithm.hpp"

using namespace kbl;
using namespace lock;

void task::fcfs_scheduler_class::enqueue(task::thread* thread)
{
	KDEBUG_ASSERT(cpu->id == parent_->owner_cpu->id);

	lock_guard lk_this{ lock_ };

	if (thread->state == thread::thread_states::DYING)
	{
		zombie_queue_.push_back(thread);
	}
	else
	{
		run_queue_.push_back(thread);
	}
}

void task::fcfs_scheduler_class::dequeue(task::thread* thread)
{
	KDEBUG_ASSERT(cpu->id == parent_->owner_cpu->id);

	lock_guard lk_this{ lock_ };

	KDEBUG_ASSERT(thread->run_queue_link.is_valid());

	if (!thread->run_queue_link.is_empty_or_detached())
	{
		run_queue_.remove(thread);
	}
}

task::thread* task::fcfs_scheduler_class::fetch()
{
	KDEBUG_ASSERT(cpu->id == parent_->owner_cpu->id);

	lock_guard lk_this{ lock_ };

	if (run_queue_.empty())
	{
		return nullptr;
	}

	auto ret = run_queue_.front_ptr();
	run_queue_.pop_front();

	return ret;
}

void task::fcfs_scheduler_class::tick()
{
	KDEBUG_ASSERT(cpu->id == parent_->owner_cpu->id);

	lock_guard lk_this{ lock_ };

	while (!zombie_queue_.empty())
	{
		auto t = zombie_queue_.front_ptr();
		zombie_queue_.pop_front();

		t->finish_dead_transition();
	}

	cur_thread->get_scheduler_state()->set_need_reschedule(true);
}

task::thread* task::fcfs_scheduler_class::steal(cpu_struct* stealer_cpu)
{

	lock_guard lk_this{ lock_ };

	if (run_queue_.empty())
	{
		return nullptr;
	}

	// check affinity_
	for (auto& t:run_queue_ | reversed)
	{
		if (cur_thread.get() != &t &&
			t.state == thread::thread_states::READY &&
			(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
			(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
		{
			if (t.scheduler_state_.affinity()->cpu == stealer_cpu->id)
			{
				run_queue_.remove(t);
				return &t;
			}
		}
	}

	for (auto& t:run_queue_ | reversed)
	{
		if (cur_thread.get() != &t &&
			t.state == thread::thread_states::READY &&
			(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
			(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
		{
			if (t.scheduler_state_.affinity()->type == cpu_affinity_type::SOFT)
			{
				run_queue_.remove(t);
				return &t;
			}
		}
	}

	// No soft-affinity_ thread available, we use hard
	for (auto& t:run_queue_ | reversed)
	{
		if (cur_thread.get() != &t &&
			t.state == thread::thread_states::READY &&
			(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
			(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
		{
			if (t.scheduler_state_.affinity()->type == cpu_affinity_type::HARD)
			{
				run_queue_.remove(t);
				return &t;
			}
		}
	}

	return nullptr;
}
task::scheduler_class::size_type task::fcfs_scheduler_class::workload_size() const
{
	lock_guard lk_this{ lock_ };

	return run_queue_.size();
}
