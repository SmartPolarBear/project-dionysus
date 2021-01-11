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

wait_queue_state::~wait_queue_state()
{

}

void wait_queue_state::block(interruptible intr, error_code block_code) TA_REQ(master_thread_lock)
{

}

error_code wait_queue_state::try_unblock(thread* t, error_code block_code) TA_REQ(master_thread_lock)
{
	return 0;
}

bool wait_queue_state::wakeup(thread* t, error_code st) TA_REQ(master_thread_lock)
{
	return false;
}

error_code_with_result<bool> wait_queue_state::try_wakeup(thread* t, error_code st) TA_REQ(master_thread_lock)
{
	return error_code_with_result<bool>();
}

void wait_queue_state::update_priority_when_blocking(thread* t, int prio, propagating prop) TA_REQ(master_thread_lock)
{

}

void owned_wait_queue::disown_all_queues(thread*)
{

}
