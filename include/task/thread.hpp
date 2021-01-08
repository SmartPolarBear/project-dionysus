#pragma once

// FIXME!
//#include "task/task_dispatcher.hpp"

#include "task/thread_state.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"

#include "ktl/shared_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

namespace task
{
	extern lock::spinlock master_thread_lock;

	class thread_dispatcher;

	using thread_start_routine = int (*)(void* arg);
	using thread_trampoline_routine = void (*)();

	class task_state final
	{
	 public:
		task_state() = default;
		void init(thread_start_routine entry, void* arg);
		error_code join(time_t deadline) TA_REQ(master_thread_lock);
		error_code wake_joiners(error_code status) TA_REQ(master_thread_lock);

		[[nodiscard]] thread_start_routine get_entry() const
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
		thread_start_routine entry{ nullptr };

		void* arg{ nullptr };
		int ret_code{ 0 };

	};

	enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_flag
	{
		THREAD_FLAG_DETACHED = (1 << 0),
		THREAD_FLAG_FREE_STRUCT = (1 << 1),
		THREAD_FLAG_IDLE = (1 << 2),
		THREAD_FLAG_VCPU = (1 << 3),
	};

	class thread final
	{
	 public:
		enum [[clang::enum_extensibility(closed)]] Status : uint8_t
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

		static void default_trampoline();

		static thread* create_idle_thread(cpu_num_type cpuid);

		static thread* create(ktl::string_view name, thread_start_routine entry, void* arg, int priority);
		static thread* create_etc(thread* t,
			const char* name,
			thread_start_routine entry,
			void* agr,
			int priority,
			thread_trampoline_routine trampoline);

	 public:
		thread();
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
		task_state task_state_;

		thread_flag flags;

		Status status;

		cpu_num_type cpuid;

		ktl::shared_ptr<thread_dispatcher> owner; // kernel thread has no owner

		lock::spinlock lock;
	};

	extern ktl::list<task::thread*> thread_list;
}