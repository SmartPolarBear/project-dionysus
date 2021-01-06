#pragma once

#include "task/task_dispatcher.hpp"
#include "task/thread_state.hpp"
#include "task/thread.hpp"

#include "kbl/lock/spinlock.h"

#include "ktl/shared_ptr.hpp"

namespace task
{
	class process_dispatcher;

	// thread is minimal execution unit
	// has its own stack and context (register values)
	class thread_dispatcher
		: public object::dispatcher<thread_dispatcher, 0>
	{
	 public:
		using thread_context_type = arch_context_registers;

		enum class [[clang::enum_extensibility(open), clang::flag_enum]] block_reasons
		{
			NONE,
			EXCEPTION,
			SLEEPING,
			FUTEX,
			PORT,
			CHANNEL,
			WAIT_ONE,
			WAIT_MANY,
			INTERRUPT,
			PAGE
		};

		friend class process_dispatcher;

		class auto_blocked final
		{
		 public:
			explicit auto_blocked(block_reasons reason)
				: thread_dispatcher_{ thread_dispatcher::get_current() },
				  prev_reason_{ thread_dispatcher_->block_reason }
			{
				thread_dispatcher_->block_reason = reason;
			}

			~auto_blocked()
			{
				thread_dispatcher_->block_reason = prev_reason_;
			}

		 private:
			thread_dispatcher* thread_dispatcher_;
			const block_reasons prev_reason_;
		};

		struct entry_status
		{
			uintptr_t pc, sp, arg1, arg2;
		};

		static error_code_with_result<ktl::shared_ptr<thread_dispatcher>> create(ktl::shared_ptr<process_dispatcher> parent,
			uint32_t flags,
			ktl::string_view name);

		static thread_dispatcher* get_current();
		[[noreturn]]static void exit_current();

		error_code initialize() TA_EXCL(lock);
		error_code start(const entry_status entry, bool initial);
		error_code mark_runnable(const entry_status entry, bool suspend);
		void kill();

		error_code suspend();
		void resume();

		bool is_dying_or_dead() const TA_EXCL(lock);
		bool has_started() const TA_EXCL(lock);

		void suspending();
		void resuming();
		void exiting_current();

		thread_dispatcher(const thread_dispatcher&) = delete;
		thread_dispatcher& operator=(const thread_dispatcher&) = delete;

	 private:
		thread_dispatcher(ktl::shared_ptr<process_dispatcher> proc, uint32_t flags);

		/// should be at the top of stack
		thread_context_type* context TA_GUARDED(lock);

		uintptr_t stack TA_GUARDED(lock);

		ktl::shared_ptr<process_dispatcher> parent TA_GUARDED(lock);

		entry_status user_entry;

		thread_status status TA_GUARDED(lock);

		volatile block_reasons block_reason;

		size_t suspend_count TA_GUARDED(lock) { 0 };

		thread* core_thread TA_GUARDED(lock){ nullptr };

		lock::spinlock lock;
	};

}