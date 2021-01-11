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

task::thread* task::thread::current::get()
{
	return task::thread::current::cur_thread.get();
}

void task::thread::current::yield()
{
	[[maybe_unused]] auto cur = thread::current::get();
	KDEBUG_ASSERT(cur->get_status() == scheduler_state::THREAD_RUNNING);

	lock_guard g{ master_thread_lock };

	scheduler2::scheduler::yield();
}

void task::thread::current::preempt()
{
	[[maybe_unused]] auto cur = thread::current::get();
	KDEBUG_ASSERT(cur->get_status() == scheduler_state::THREAD_RUNNING);

	lock_guard g{ master_thread_lock };

	scheduler2::scheduler::preempt();
}

void task::thread::current::reschedule()
{
	[[maybe_unused]] auto cur = thread::current::get();
	KDEBUG_ASSERT(cur->get_status() == scheduler_state::THREAD_RUNNING);

	lock_guard g{ master_thread_lock };

	scheduler2::scheduler::reschedule();
}

void task::thread::current::exit(int retcode)
{
	auto cur = thread::current::get();
	KDEBUG_ASSERT(cur->get_status() == scheduler_state::THREAD_RUNNING);
	KDEBUG_ASSERT(!cur->is_idle());

	if (cur->owner)
	{
		KDEBUG_ASSERT(arch_ints_disabled() == false || !master_thread_lock.holding());
		cur->owner->exiting_current();
	}

	lock_guard g{ master_thread_lock };
	thread::current::exit_locked(retcode);
}

void task::thread::current::exit_locked(int retcode) TA_REQ(lock)
{

}

void task::thread::current::becomde_idle()
{

}

void task::thread::current::do_suspend()
{

}

void task::thread::current::set_name(const char* name)
{

}