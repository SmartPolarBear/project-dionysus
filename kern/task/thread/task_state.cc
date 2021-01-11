#include "process.hpp"

#include "drivers/apic/traps.h"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"
#include "task/thread/thread_dispatcher.hpp"
#include "task/process/process_dispatcher.hpp"

#include "system/process.h"
#include "system/dpc.hpp"

#include "kbl/lock/spinlock.h"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/algorithm.hpp"

using namespace lock;
using namespace task;
using namespace task::scheduler2;

using ktl::mutex::lock_guard;

void task::task_state::init(task::thread_start_routine_type entry, void* arg)
{

}

error_code task::task_state::join(time_t deadline) TA_REQ(master_thread_lock)
{
	return 0;
}

error_code task::task_state::wake_joiners(error_code status) TA_REQ(master_thread_lock)
{
	return 0;
}
