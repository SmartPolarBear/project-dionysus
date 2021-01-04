#pragma once

#include "task/task_dispatcher.hpp"

#include "kbl/lock/spinlock.h"

namespace task
{

	// thread is minimal execution unit
	// has its own stack and context (register values)
	class thread_dispatcher
	{
	 public:
		using thread_context_type = arch_context_registers;

		friend class process_dispatcher;
	 private:
		thread_context_type context TA_GUARDED(lock);
		uintptr_t stack;

		std::shared_ptr<process_dispatcher> parent TA_GUARDED(lock);

		lock::spinlock lock;
	};
}