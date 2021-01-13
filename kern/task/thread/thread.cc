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
thread::thread_list_type task::global_thread_list{};
cls_item<thread*, CLS_CUR_THREAD_PTR> task::cur_thread;

ktl::unique_ptr<kernel_stack> task::kernel_stack::create(thread::routine_type start_routine,
	void* arg,
	thread::trampoline_type tpl)
{
	kbl::allocate_checker ck{};
	ktl::unique_ptr<kernel_stack> ret{ new(&ck) kernel_stack{ start_routine, arg, tpl }};

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

kernel_stack::kernel_stack(thread::routine_type routine, void* arg, thread::trampoline_type tpl)
{
	// setup initial kernel stack
	auto sp = reinterpret_cast<uintptr_t>(static_cast<char*>(top) + task::process::KERNSTACK_SIZE);

	sp -= sizeof(trap::trap_frame);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);
	memset(this->tf, 0, sizeof(*this->tf));

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(thread_trampoline_s);

	sp -= sizeof(arch_task_context_registers);
	this->context = reinterpret_cast<arch_task_context_registers*>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)tpl;
	this->tf->rip = reinterpret_cast<uintptr_t >(thread_entry);

	// setup registers
	tf->cs = SEGMENT_VAL(SEGMENTSEL_KCODE, DPL_KERNEL);
	tf->ss = SEGMENT_VAL(SEGMENTSEL_KDATA, DPL_KERNEL);

	tf->rflags |= trap::EFLAG_IF | trap::EFLAG_IOPL_MASK;

	// argument: routine arg and exit callback
	tf->rdi = reinterpret_cast<uintptr_t>(routine);
	tf->rsi = reinterpret_cast<uintptr_t>(arg);
	tf->rdx = reinterpret_cast<uintptr_t>(thread::current::exit);
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

	ret->kstack = kernel_stack::create(routine, nullptr, trampoline);

	if (ret->kstack == nullptr)
	{
		delete ret;
		return -ERROR_MEMORY_ALLOC;
	}

	return ret;
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

void thread::switch_to()
{

	KDEBUG_ASSERT(this->state == thread_states::READY);
	KDEBUG_ASSERT(this->get_mm() != nullptr);

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
