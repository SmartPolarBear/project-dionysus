#include "task/scheduler/scheduler.hpp"

#include "system/scheduler.h"

// TODO: temporarily link to old scheduler

void task::scheduler::reschedule()
{
	::scheduler::scheduler_enter();
}

void task::scheduler::yield()
{
	::scheduler::scheduler_yield();
}
