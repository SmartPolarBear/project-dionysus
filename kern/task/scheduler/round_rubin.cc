#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{
	run_queue.push_back(thread);
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
	cur_thread->need_reschedule = true;
}
