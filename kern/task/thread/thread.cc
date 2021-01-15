#include "../include/syscall.h"
#include "internals/thread.hpp"

#include "task/thread/thread.hpp"
#include "task/process/process.hpp"
#include "task/scheduler/scheduler.hpp"

#include "system/mmu.h"
#include "system/vmm.h"
#include "system/kmalloc.hpp"
#include "system/scheduler.h"

#include "drivers/acpi/cpu.h"

#include "ktl/mutex/lock_guard.hpp"

#include <utility>

using namespace task;

using ktl::mutex::lock_guard;

lock::spinlock task::global_thread_lock{ "gtl" };
cls_item<thread*, CLS_CUR_THREAD_PTR> task::cur_thread TA_GUARDED(global_thread_lock);
thread::thread_list_type task::global_thread_list TA_GUARDED(global_thread_lock);

ktl::unique_ptr<kernel_stack> task::kernel_stack::create(thread::routine_type start_routine,
	void* arg,
	thread::trampoline_type tpl)
{
	kbl::allocate_checker ck{};

	auto stack_mem = memory::kmalloc(MAX_SIZE, 0);
	ktl::unique_ptr<kernel_stack> ret{ new(&ck) kernel_stack{ stack_mem, start_routine, arg, tpl }};

	if (!ck.check())
	{
		return nullptr;
	}

	if (ret->bottom == nullptr)
	{
		return nullptr;
	}

	return ret;
}

kernel_stack::~kernel_stack()
{
	memory::kfree(bottom);
}

kernel_stack::kernel_stack(void* stk_mem, thread::routine_type routine, void* arg, thread::trampoline_type tpl)
	: bottom(stk_mem)
{
	// setup initial kernel stack
	auto sp = reinterpret_cast<uintptr_t>(static_cast<char*>(bottom) + MAX_SIZE);

	sp -= sizeof(trap::trap_frame);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);
	memset(this->tf, 0, sizeof(*this->tf));

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(thread_trampoline_s);

	sp -= sizeof(arch_task_context_registers);
	this->context = reinterpret_cast<arch_task_context_registers*>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)tpl;

	// setup registers
	tf->cs = SEGMENT_VAL(SEGMENTSEL_KCODE, DPL_KERNEL);
	tf->ss = SEGMENT_VAL(SEGMENTSEL_KDATA, DPL_KERNEL);

	tf->rflags |= trap::EFLAG_IF | trap::EFLAG_IOPL_MASK;

	// argument: routine arg and exit callback
	tf->rdi = reinterpret_cast<uintptr_t>(routine);
	tf->rsi = reinterpret_cast<uintptr_t>(arg);
	tf->rdx = reinterpret_cast<uintptr_t>(thread::current::exit);

	tf->rip = reinterpret_cast<uintptr_t >(thread_entry);

	top = sp;
}

error_code_with_result<task::thread*> thread::create(ktl::shared_ptr<process> parent,
	ktl::string_view name,
	routine_type routine,
	void* arg,
	trampoline_type trampoline)
{

	kbl::allocate_checker ck{};
	auto ret = new(&ck) thread{ std::move(parent), name };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ret->kstack = kernel_stack::create(routine, arg, trampoline);

	if (ret->kstack == nullptr)
	{
		delete ret;
		return -ERROR_MEMORY_ALLOC;
	}

	if (parent != nullptr)
	{
		// TODO: allocate user stack
		KDEBUG_NOT_IMPLEMENTED;
	}
	else
	{
		ret->kstack->tf->rsp = ret->kstack->top;
	}

	ret->state = thread_states::READY;
	return ret;
}

error_code_with_result<task::thread*> thread::create_and_enqueue(ktl::shared_ptr<process> parent,
	ktl::string_view name,
	thread::routine_type routine,
	void* arg,
	thread::trampoline_type trampoline)
{
	lock_guard g{ global_thread_lock };

	auto create_ret = create(std::move(parent), name, routine, arg, trampoline);
	if (has_error(create_ret))
	{
		return get_error_code(create_ret);
	}

	auto t = get_result(create_ret);

	global_thread_list.push_back(*t);

	return t;
}

error_code thread::create_idle()
{
	[[maybe_unused]]auto ret = create_and_enqueue(nullptr, "idle", idle_routine, nullptr, default_trampoline);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	lock_guard g{ global_thread_lock };

	auto th = get_result(ret);

	th->flag |= FLAG_IDLE;

	if (cur_thread == nullptr)
	{
		cur_thread = th;
	}

	return ERROR_SUCCESS;
}

vmm::mm_struct* task::thread::get_mm()
{
	if (parent != nullptr)
	{
		return parent->get_mm();
	}

	return nullptr;
}

void thread::default_trampoline()
{
	global_thread_lock.unlock();

	// will return to thread_entry
}

void thread::switch_to() TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(this->state == thread_states::READY);

	trap::pushcli();

	cur_thread = this;

	cur_thread->state = thread_states::RUNNING;

	// TODO: add counters to track run count

	auto mm = this->get_mm();
	if (mm == nullptr)
	{
		lcr3(V2P((uintptr_t)vmm::g_kpml4t));
	}
	else
	{
		lcr3(V2P((uintptr_t)this->get_mm()->pgdir));
	}

	uintptr_t kstack_addr = this->kstack->get_address();

	cpu->tss.rsp0 = kstack_addr + kernel_stack::MAX_SIZE;

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

void thread::remove_from_lists()
{
	lock_guard g{ global_thread_lock };
	global_thread_list.erase(thread_list_type::iterator_type{ &this->thread_link });
}

void thread::finish_dying()
{
	cur_thread = nullptr;
	this->state = thread_states::DEAD;

	delete this;
}
error_code thread::idle_routine(void* arg)
{
	while (true)hlt();
	return ERROR_SUCCESS;
}

void thread::current::exit(error_code code)
{
	lock_guard g{ global_thread_lock };

	cur_thread->parent->remove_thread(cur_thread.get());

	cur_thread->state = thread_states::DYING;
	cur_thread->exit_code = code;

	if (cur_thread->flag & FLAG_DETACHED)
	{
		cur_thread->remove_from_lists();
	}
	else
	{
		// TODO: waiting queue
	}

	task::scheduler::reschedule();

	__UNREACHABLE;
}
