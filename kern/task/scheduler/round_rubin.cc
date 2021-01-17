#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

#include "ktl/mutex/lock_guard.hpp"

#include "ktl/algorithm.hpp"

using namespace ktl::mutex;

void task::round_rubin_scheduler_class::enqueue(task::thread* thread)
{

}

void task::round_rubin_scheduler_class::dequeue(task::thread* thread)
{

}

task::thread* task::round_rubin_scheduler_class::pick_next()
{
	return nullptr;
}

void task::round_rubin_scheduler_class::timer_tick()
{

}
