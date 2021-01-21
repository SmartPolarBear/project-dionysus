#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{

	run_queue.push_front(thread);
}

void task::round_rubin_scheduler_class::dequeue(task::thread* thread)
{
	run_queue.remove(thread);
}

task::thread* task::round_rubin_scheduler_class::pick_next()
{
	if (current == run_queue.end() && !run_queue.empty())
	{
		current = run_queue.begin();
	}

	if (!run_queue.empty())
	{
		return *(current++);
	}
	else if (run_queue.empty())
	{
		current = run_queue.end();
		return nullptr;
	}

	__UNREACHABLE;
	return nullptr;
}

void task::round_rubin_scheduler_class::timer_tick()
{
	lock_guard g{ cur_thread->lock };

	cur_thread->need_reschedule = true;
}
