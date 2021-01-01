#pragma once

#include "task/task_dispatcher.hpp"

namespace task
{

	// stack is minimal execution unit
	// has its own stack and context (register values)
	class thread
	{
	 public:
		using thread_context = arch_context_registers;
		using stack_pointer = ktl::unique_ptr<uint8_t[]>;

	 private:
		context context;
		stack_pointer stack;

	};
}