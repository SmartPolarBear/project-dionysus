#pragma once

#include "task/task_dispatcher.hpp"
#include "task/thread_state.hpp"

#include "kbl/lock/spinlock.h"

#include "ktl/shared_ptr.hpp"
#include "ktl/string_view.hpp"

namespace task
{
	using thread_start_routine = int (*)(void* arg);
	using thread_trampoline_routine = void (*)();

	class thread final
	{
	 public:
		static thread* create_idle_thread(cpu_num_type cpuid);

		static thread* create(ktl::string_view name,thread_start_routine entry,void *arg,)

	 public:
		thread();
		~thread();

	};
}