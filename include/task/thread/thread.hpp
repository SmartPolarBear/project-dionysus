#pragma once

// FIXME!
//#include "task/task_dispatcher.hpp"

#include "thread_state.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/atomic/atomic.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

namespace task
{
extern lock::spinlock master_thread_lock;

class thread_dispatcher;

using thread_start_routine_type = int (*)(void* arg);
using thread_trampoline_routine_type = void (*)();

enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_flags
{
	THREAD_FLAG_DETACHED = (1 << 0),
	THREAD_FLAG_FREE_STRUCT = (1 << 1),
	THREAD_FLAG_IDLE = (1 << 2),
	THREAD_FLAG_VCPU = (1 << 3),
};

enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_signals
{
	THREAD_SIGNAL_KILL = (1 << 0),
	THREAD_SIGNAL_SUSPEND = (1 << 1),
	THREAD_SIGNAL_POLICY_EXCEPTION = (1 << 2),
};

class wait_queue final
{
 public:
 private:
};

class task_state final
{
 public:
	task_state() = default;
	void init(thread_start_routine_type entry, void* arg);
	error_code join(time_t deadline) TA_REQ(master_thread_lock);
	error_code wake_joiners(error_code status) TA_REQ(master_thread_lock);

	[[nodiscard]] thread_start_routine_type get_entry() const
	{
		return entry;
	}

	[[nodiscard]] void* get_arg() const
	{
		return arg;
	}

	[[nodiscard]] int get_ret_code() const
	{
		return ret_code;
	}

	void set_ret_code(int rc)
	{
		task_state::ret_code = rc;
	}
 private:
	thread_start_routine_type entry{ nullptr };

	void* arg{ nullptr };
	int ret_code{ 0 };

};

class scheduler_state final
{
 public:
	enum [[clang::enum_extensibility(closed)]] thread_status : uint8_t
	{
		THREAD_INITIAL = 0,
		THREAD_READY,
		THREAD_RUNNING,
		THREAD_BLOCKED,
		THREAD_BLOCKED_READ_LOCK,
		THREAD_SLEEPING,
		THREAD_SUSPENDED,
		THREAD_DEATH,
	};

 public:
	[[nodiscard]] thread_status get_status() const
	{
		return status;
	}

	void set_status(thread_status s)
	{
		scheduler_state::status = s;
	}

 private:
	thread_status status;

};

class thread final
{
 public:
	using list_type = ktl::list<task::thread*>;

	static void default_trampoline();

	static thread* create_idle_thread(cpu_num_type cpuid);

	static thread* create(ktl::string_view name, thread_start_routine_type entry, void* arg, int priority);
	static thread* create_etc(thread* t,
		ktl::string_view name,
		thread_start_routine_type entry,
		void* agr,
		int priority,
		thread_trampoline_routine_type trampoline);

 public:
	thread();
	explicit thread(ktl::string_view name);
	~thread();

	void resume();
	error_code suspend();
	void forget();

	error_code detach();
	error_code detach_and_resume();

	error_code join(int* retcode, time_t deadline);

	void kill();

	void erase_from_all_lists() TA_REQ(master_thread_lock);

	void set_priority(int priority);

	ktl::string_view get_owner_name();

	struct current
	{
		static inline thread* get();

		static void yield();
		static void preempt();
		static void reschedule();
		[[noreturn]]static void exit(int retcode);
		[[noreturn]]static void exit_locked(int retcode) TA_REQ(lock);
		[[noreturn]]static void becomde_idle();

		static void do_suspend();
		static void set_name(const char* name);
	};

 private:
	kbl::integral_atomic<uint64_t> signals{};

	wait_queue wait_queue_{};

	task_state task_state_{};

	int64_t flags{ 0 };

	scheduler_state scheduler_state_{};

	cpu_num_type cpuid{ CPU_NUM_INVALID };

	ktl::shared_ptr<thread_dispatcher> owner{ nullptr }; // kernel thread has no owner

	ktl::string_view name{ "" };

	mutable lock::spinlock lock{ "thread_own_lock" };
};

extern thread::list_type thread_list;
}