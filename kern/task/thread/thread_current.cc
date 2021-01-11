#include "process.hpp"

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

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

extern int idle_thread_start_routine(void* arg);

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
	auto cur = thread::current::get();

	cur->set_death();
	cur->task_state_.set_ret_code(retcode);

	owned_wait_queue::disown_all_queues(cur);

	if (cur->flags & THREAD_FLAG_DETACHED)
	{
		cur->erase_from_all_lists();

		if (cur->flags & THREAD_FLAG_RELEASE_NEEDED || cur->kstack.get())
		{
			dpc free_dpc{ thread::free_dpc, cur };
			auto ret = free_dpc.queue_thread_locked();
			if (ret != ERROR_SUCCESS)
			{
				KDEBUG_GENERALPANIC(ret);
			}
		}
	}
	else
	{
		cur->task_state_.wake_joiners(ERROR_SUCCESS);
	}

	scheduler2::scheduler::reschedule_internal();

	KDEBUG_GERNERALPANIC_CODE(-ERROR_SHOULD_NOT_REACH_HERE);
}

void task::thread::current::becomde_idle()
{
	KDEBUG_ASSERT(arch_ints_disabled());

	auto t = thread::current::get();
	auto cpuid = cpu->id;

	char name[16] = { 0 };
	sniprintf(name, sizeof(name), "idle %u", cpuid);
	thread::current::set_name(name);

	t->flags |= THREAD_FLAG_IDLE;
	scheduler2::scheduler::initialize_thread(t, 0);

	{
		lock_guard g{ master_thread_lock };

		scheduler2::scheduler::reschedule();
	}

	sti();

	idle_thread_start_routine(nullptr);

	KDEBUG_GERNERALPANIC_CODE(-ERROR_SHOULD_NOT_REACH_HERE);
}

void task::thread::current::do_suspend()
{
	auto cur = thread::current::get();

	if (cur->owner)
	{
		KDEBUG_ASSERT(!arch_ints_disabled() || !master_thread_lock.holding());
		cur->owner->on_suspending();
	}

	{
		lock_guard g{ master_thread_lock };

		// make sure we haven't been killed while the lock was dropped for the user callback
		if (cur->check_kill_signal())
		{
			g.unlock();
			thread::current::exit(ERROR_SUCCESS);
		}

		if (cur->get_signals() & THREAD_SIGNAL_SUSPEND)
		{
			cur->set_suspended();
			cur->signals_.fetch_and(~THREAD_SIGNAL_SUSPEND, kbl::memory_order_relaxed);

			scheduler2::scheduler::reschedule_internal();

			if (cur->check_kill_signal())
			{
				g.unlock();
				thread::current::exit(ERROR_SUCCESS);
			}
		}
	}

	if (cur->owner)
	{
		KDEBUG_ASSERT(!arch_ints_disabled() || !master_thread_lock.holding());
		cur->owner->on_resuming();
	}
}

void task::thread::current::set_name(const char* n)
{
	auto cur = thread::current::get();
	cur->name = n;
}