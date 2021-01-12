#include "../include/syscall.h"

#include "task/thread/thread.hpp"
#include "task/process/process.hpp"

#include "internals/thread.hpp"

#include "system/mmu.h"
#include "system/kmalloc.hpp"

#include "drivers/acpi/cpu.h"

#include <utility>

using namespace task;

lock::spinlock task::global_thread_lock{ "gtl" };
thread::thread_list_type task::global_thread_list{};
cls_item<thread*, CLS_CUR_THREAD_PTR> task::cur_thread;

ktl::unique_ptr<kernel_stack> task::kernel_stack::create(thread::routine_type start_routine,
	thread::trampoline_type tpl)
{
	kbl::allocate_checker ck{};
	ktl::unique_ptr<kernel_stack> ret{ new(&ck) kernel_stack{ start_routine, tpl }};

	if (!ck.check())
	{
		return nullptr;
	}

	ret->top = memory::kmalloc(KERNEL_SIZE, 0);
	if (ret->top == nullptr)
	{
		return nullptr;
	}

	return ret;
}

kernel_stack::~kernel_stack()
{
	memory::kfree(top);
}

kernel_stack::kernel_stack(thread::routine_type routine, thread::trampoline_type tpl)
{
	// setup initial kernel stack
	auto sp = reinterpret_cast<uintptr_t>(static_cast<char*>(top) + task::process::KERNSTACK_SIZE);

	sp -= sizeof(trap::trap_frame);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(user_proc_entry);

	sp -= sizeof(arch_task_context_registers);
	this->context = reinterpret_cast<arch_task_context_registers*>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)tpl;

	// setup tf to execute the routine
	this->tf->rip = reinterpret_cast<uintptr_t >(routine);
}

error_code_with_result<thread*> thread::create(ktl::shared_ptr<process> parent,
	ktl::string_view name,
	routine_type routine,
	trampoline_type trampoline)
{

	kbl::allocate_checker ck{};
	auto ret = new(&ck) thread{ std::move(parent), name };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ret->kstack = kernel_stack::create(routine, trampoline);

	if (ret->kstack == nullptr)
	{
		delete ret;
		return -ERROR_MEMORY_ALLOC;
	}

	return ret;
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

	context_switch(&cpu->scheduler, this->kstack->context);

	cur_thread = nullptr;
}

thread::thread(ktl::shared_ptr<process> prt,
	ktl::string_view nm)
	: parent{ std::move(prt) },
	  name{ nm }
{
}
