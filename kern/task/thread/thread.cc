#include "process.hpp"

#include "drivers/apic/traps.h"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"
#include "task/thread/thread_dispatcher.hpp"
#include "task/process/process_dispatcher.hpp"

#include "system/process.h"

#include "kbl/lock/spinlock.h"

#include "ktl/mutex/lock_guard.hpp"

using namespace lock;
using namespace task;
using namespace task::scheduler2;

using ktl::mutex::lock_guard;

spinlock task::master_thread_lock{ "thread master lock" };
ktl::list<task::thread*> task::thread_list TA_GUARDED(master_thread_lock);

// TODO: arch dependent
int idle_thread_start_routine(void* arg)
{
	hlt();
}

void thread_initialize(thread* t, thread_trampoline_routine_type trampoline)
{

}

// TODO: end arch dependent

void task::thread::default_trampoline()
{
	proc_list.lock.unlock();
	master_thread_lock.unlock();
}

task::thread::thread() = default;

thread::thread(ktl::string_view n) :
	name{ n }
{
}

task::thread::~thread() = default;

task::thread* task::thread::create_idle_thread(cpu_num_type cpuid)
{
	KDEBUG_ASSERT(cpuid != 0); // not boot cpu

	char name[16] = { 0 };
	sniprintf(name, sizeof(name), "idle %u", cpuid);

	auto th = thread::create_etc(&cpus[cpuid].idle, name, idle_thread_start_routine, nullptr, 0, nullptr);

	if (th == nullptr)
	{
		return th;
	}

	th->flags = static_cast<thread_flags>(THREAD_FLAG_IDLE | THREAD_FLAG_DETACHED);

	// TODO: these should be scheduler's job?
	th->scheduler_state_.set_status(scheduler_state::THREAD_INITIAL);
	th->cpuid = cpuid;

	return th;
}

task::thread* task::thread::create_etc(task::thread* t,
	ktl::string_view name,
	task::thread_start_routine_type entry,
	void* arg,
	[[maybe_unused]]int priority,
	task::thread_trampoline_routine_type trampoline)
{
	size_t flags{ 0 };
	kbl::allocate_checker ck{};
	if (t)
	{
		t = new(&ck) thread{ name };
		if (!ck.check())
		{
			return nullptr;
		}

		flags |= THREAD_FLAG_FREE_STRUCT;
	}

	t->task_state_.init(entry, arg);
	scheduler2::scheduler::initialize_thread(t, priority);

	if (likely(trampoline == nullptr))
	{
		trampoline = &thread::default_trampoline;
	}

	thread_initialize(t, trampoline);

	{
		ktl::mutex::lock_guard g{ master_thread_lock };
		thread_list.push_back(t);
	}

	return nullptr;
}

task::thread* task::thread::create(ktl::string_view name,
	task::thread_start_routine_type entry,
	void* arg,
	int priority)
{
	return create_etc(nullptr, name, entry, arg, priority, nullptr);
}

void task::thread::resume()
{
	auto rflags = read_rflags();
	bool if_flag = (rflags & trap::EFLAG_IF), resched = false;

	// never reschedule into boot thread before idle thread set up
	if (if_flag)
	{
		resched = true;
	}

	{
		lock_guard g{ master_thread_lock };

		if (this->scheduler_state_.get_status() == scheduler_state::THREAD_DEATH)
		{
			return;
		}

		signals.fetch_and(~THREAD_SIGNAL_SUSPEND, kbl::memory_order_relaxed);

		if (auto st = scheduler_state_.get_status();st == scheduler_state::THREAD_INITIAL
			|| st == scheduler_state::THREAD_SUSPENDED)
		{
			bool local_resched = scheduler2::scheduler::unblock(this);
			if (resched && local_resched)
			{
				scheduler2::scheduler::reschedule();
			}
		}
	}
}

error_code task::thread::suspend()
{
	if (is_idle())
	{
		return -ERROR_INVALID;
	}

	lock_guard g{ master_thread_lock };

	if (scheduler_state_.get_status() == scheduler_state::THREAD_DEATH)
	{
		return -ERROR_THREAD_STATE;
	}

	signals.fetch_or(THREAD_SIGNAL_SUSPEND, kbl::memory_order_relaxed);



	return 0;
}

void task::thread::forget()
{

}

error_code task::thread::detach()
{
	return 0;
}

error_code task::thread::detach_and_resume()
{
	if (auto err = detach();err != ERROR_SUCCESS)
	{
		return err;
	}
	resume();
	return ERROR_SUCCESS;
}

error_code task::thread::join(int* ret_code, time_t deadline)
{
	return 0;
}

void task::thread::kill()
{

}

void task::thread::erase_from_all_lists() TA_REQ(master_thread_lock)
{

}

void task::thread::set_priority(int priority)
{

}

ktl::string_view task::thread::get_owner_name()
{
	if (owner != nullptr)
	{
		return owner->get_parent()->get_name();
	}

	return "kernel";
}

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
task::thread* task::thread::current::get()
{
	return nullptr;
}
void task::thread::current::yield()
{

}
void task::thread::current::preempt()
{

}
void task::thread::current::reschedule()
{

}
void task::thread::current::exit(int retcode)
{

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
