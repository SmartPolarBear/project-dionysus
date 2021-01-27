#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "drivers/acpi/cpu.h"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

size_t task::round_rubin_scheduler_class::called_count = 0;

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{

	if (thread->state == thread::thread_states::DYING)
	{
		zombie_queue.push_back(thread);
	}
	else
	{
		run_queue.push_back(thread);
		int a = 0;
	}
}

void task::round_rubin_scheduler_class::dequeue(task::thread* thread)
{
	KDEBUG_ASSERT(thread->run_queue_link.next != nullptr && thread->run_queue_link.prev != nullptr);

	if (thread->run_queue_link.next != &thread->run_queue_link
		&& thread->run_queue_link.prev != &thread->run_queue_link)
	{
		run_queue.remove(thread);
		int a = 0;
	}
}

task::thread* task::round_rubin_scheduler_class::pick_next()
{
//	if (run_queue.size_ == 0 && !run_queue.empty())
//	{
//		run_queue.head_.next = &run_queue.head_;
//		run_queue.head_.prev = &run_queue.head_;
//	}
	if (run_queue.empty())
	{
		for (size_t i = 0; i < CPU_COUNT_LIMIT; i++)
		{
			kdebug::kdebug_log("*cpu %d:%d\n" + (this->owner_cpu != cpus[i].scheduler.scheduler_class.owner_cpu),
				i,
				cpus[i].scheduler.scheduler_class.run_queue.size());
		}
		return nullptr;
	}

	auto ret = run_queue.front_ptr();
	run_queue.pop_front();

	return ret;
}

void task::round_rubin_scheduler_class::timer_tick()
{

	while (!zombie_queue.empty())
	{
		auto t = zombie_queue.front_ptr();
		zombie_queue.pop_front();

		t->finish_dead_transition();
	}

	cur_thread->need_reschedule = true;
}
