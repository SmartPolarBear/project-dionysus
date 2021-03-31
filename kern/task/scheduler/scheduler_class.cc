#include "internals/thread.hpp"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"

#include "drivers/acpi/cpu.h"

#include "system/scheduler.h"

#include "kbl/data/utility.hpp"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/algorithm.hpp"

using namespace kbl;
using namespace task;

thread* task::scheduler_class::steal(cpu_struct* stealer_cpu) TA_REQ(global_thread_lock)
{
	return nullptr;
}