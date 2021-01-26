#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

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
	run_queue.remove(*thread);
}

task::thread* task::round_rubin_scheduler_class::pick_next()
{
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
