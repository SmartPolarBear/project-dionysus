#include "task/thread/thread.hpp"
#include "task/process/process.hpp"

#include "internals/thread.hpp"

#include "system/mmu.h"

using namespace task;

lock::spinlock task::global_thread_lock{ "gtl" };
thread::thread_list_type task::global_thread_list{};
cls_item<thread*, CLS_CUR_THREAD_PTR> task::cur_thread;

ktl::unique_ptr<kernel_stack> task::kernel_stack::create(vmm::mm_struct* parent_mm,
	thread::routine_type start_routine,
	thread::trampoline_type tpl)
{
	return ktl::unique_ptr<kernel_stack>();
}

error_code kernel_stack::grow_downward(size_t count)
{
	return 0;
}

thread* thread::create(ktl::shared_ptr<process> parent,
	ktl::string_view name,
	routine_type routine,
	trampoline_type trampoline)
{
	return nullptr;
}

vmm::mm_struct* task::thread::get_mm()
{
	return parent->get_mm();
}

void thread::trampoline()
{
	global_thread_lock.unlock();

	// will return to thread_entry
}

void thread::switch_to()
{
	cur_thread = this;
	context_switch(&cpu->scheduler, &cur_thread->context);
}
