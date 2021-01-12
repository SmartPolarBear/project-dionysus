#include "task/thread/thread.hpp"
#include "task/process/process.hpp"

#include "system/mmu.h"

using namespace task;

ktl::shared_ptr<kernel_stack> task::kernel_stack::create(vmm::mm_struct* parent_mm, thread::trampoline_type tpl)
{
	return ktl::shared_ptr<kernel_stack>();
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

}
void thread::switch_to()
{

}
