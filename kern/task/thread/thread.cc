#include "../include/syscall.h"

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
	KDEBUG_ASSERT(this->state == thread_states::READY);
	KDEBUG_ASSERT(this->get_mm() != nullptr);

	trap::pushcli();

	cur_thread = this;

	cur_thread->state = thread_states::RUNNING;

	// TODO: add counters to track run count?

	lcr3(V2P((uintptr_t)this->get_mm()->pgdir));

	uintptr_t kstack_addr = this->kstack->get_address();

	cpu->tss.rsp0 = kstack_addr + task::process::KERNSTACK_SIZE;

	// Set gs. without calling swapgs to ensure atomic
	gs_put_cpu_dependent(KERNEL_GS_KSTACK, kstack_addr);

	trap::popcli();

	context_switch(&cpu->scheduler, &this->context);

	cur_thread = nullptr;
}
