#pragma once

#include "task/task_dispatcher.hpp"
#include "task/thread_state.hpp"

#include "kbl/lock/spinlock.h"

namespace task
{

	// thread is minimal execution unit
	// has its own stack and context (register values)
	class thread_dispatcher
		: public dispatcher<thread_dispatcher, 0>
	{
	 public:
		using thread_context_type = arch_context_registers;

		friend class process_dispatcher;

		enum class [[clang::enum_extensibility(open), clang::flag_enum]] block_status
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

		struct entry_status
		{
			uintptr_t pc, sp, arg1, arg2;
		};

		thread_dispatcher(const thread_dispatcher&) = delete;
		thread_dispatcher& operator=(const thread_dispatcher&) = delete;
	 private:
		thread_dispatcher(ktl::shared_ptr<process_dispatcher> proc, uint32_t flags);


		thread_context_type context TA_GUARDED(lock);
		uintptr_t stack TA_GUARDED(lock);

		ktl::shared_ptr<process_dispatcher> parent TA_GUARDED(lock);

		entry_status user_entry;


		lock::spinlock lock;
	};
}