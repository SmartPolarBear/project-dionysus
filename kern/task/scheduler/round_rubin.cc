#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "drivers/acpi/cpu.h"

#include "system/scheduler.h"

#include "kbl/data/utility.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

using namespace kbl;

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{
	if (thread->state == thread::thread_states::DYING)
	{
		zombie_queue.push_back(thread);
	}
	else
	{
		run_queue.push_back(thread);
	}
}

void task::round_rubin_scheduler_class::dequeue(task::thread* thread)
{
	KDEBUG_ASSERT(thread->run_queue_link.next != nullptr && thread->run_queue_link.prev != nullptr);

	if (thread->run_queue_link.next != &thread->run_queue_link
		&& thread->run_queue_link.prev != &thread->run_queue_link)
	{
		run_queue.remove(thread);
	}
}

task::thread* task::round_rubin_scheduler_class::fetch()
{

	if (run_queue.empty())
	{
		return nullptr;
	}

	auto ret = run_queue.front_ptr();
	run_queue.pop_front();

	return ret;
}

void task::round_rubin_scheduler_class::tick()
{
	while (!zombie_queue.empty())
	{
		auto t = zombie_queue.front_ptr();
		zombie_queue.pop_front();

		t->finish_dead_transition();
	}

	cur_thread->need_reschedule_ = true;
}

task::thread* task::round_rubin_scheduler_class::steal(cpu_struct* stealer_cpu)
{
	KDEBUG_ASSERT(global_thread_lock.holding());

	if (!run_queue.empty())
	{
		// check affinity_
		for (auto& t:run_queue | reversed)
		{
			if (cur_thread.get() != &t &&
				t.state == thread::thread_states::READY &&
				(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
				(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
			{
				if (t.affinity_.cpu == stealer_cpu->id)
				{
					run_queue.remove(t);
					return &t;
				}
			}
		}

		for (auto& t:run_queue | reversed)
		{
			if (cur_thread.get() != &t &&
				t.state == thread::thread_states::READY &&
				(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
				(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
			{
				if (t.affinity_.type == cpu_affinity_type::SOFT)
				{
					run_queue.remove(t);
					return &t;
				}
			}
		}

		// No soft-affinity_ thread available, we use hard
		for (auto& t:run_queue | reversed)
		{
			if (cur_thread.get() != &t &&
				t.state == thread::thread_states::READY &&
				(t.flags_ & thread::thread_flags::FLAG_IDLE) == 0 &&
				(t.flags_ & thread::thread_flags::FLAG_INIT) == 0)
			{
				if (t.affinity_.type == cpu_affinity_type::HARD)
				{
					run_queue.remove(t);
					return &t;
				}
			}
		}
	}

	return nullptr;
}
task::scheduler_class_base::size_type task::round_rubin_scheduler_class::workload_size() const
{
	return run_queue.size();
}
