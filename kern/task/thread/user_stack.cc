#include "task/thread/user_stack.hpp"

using namespace task;

user_stack::user_stack(process* p, thread* t, void* stack_ptr)
	: top{ stack_ptr }, owner_process{ p }, owner_thread{ t }
{
}

void* user_stack::get_top()
{
	return top;
}
