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

kernel_stack* task::kernel_stack::create(thread* parent,
	thread::routine_type start_routine,
	void* arg,
	thread::trampoline_type tpl)
{
	kbl::allocate_checker ck{};

	auto stack_mem = memory::kmalloc(MAX_SIZE, 0);
	auto ret = new(&ck) kernel_stack{ parent, stack_mem, start_routine, arg, tpl };

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

kernel_stack::kernel_stack(thread* parent_thread,
	void* stk_mem,
	thread::routine_type routine,
	void* arg,
	thread::trampoline_type tpl)
	: parent(parent_thread), bottom(stk_mem)
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

	tf->rflags |= trap::EFLAG_IF | trap::EFLAG_IOPL_MASK;

	// argument: routine arg and exit callback
	tf->rdi = reinterpret_cast<uintptr_t>(routine);
	tf->rsi = reinterpret_cast<uintptr_t>(arg);
	tf->rdx = reinterpret_cast<uintptr_t>(thread::current::exit);

	tf->rip = reinterpret_cast<uintptr_t >(thread_entry);

	// setup registers
	if (parent_thread->parent != nullptr) // the thread is for user
	{
		tf->cs = SEGMENT_VAL(SEGMENTSEL_UCODE, DPL_USER);
		tf->ss = SEGMENT_VAL(SEGMENTSEL_UDATA, DPL_USER);

		tf->rsp = USER_STACK_TOP;
	}
	else // kernel
	{
		tf->cs = SEGMENT_VAL(SEGMENTSEL_KCODE, DPL_KERNEL);
		tf->ss = SEGMENT_VAL(SEGMENTSEL_KDATA, DPL_KERNEL);

		tf->rsp = sp;
	}
}

error_code_with_result<task::thread*> thread::create(process* parent,
	ktl::string_view name,
	routine_type routine,
	void* arg,
	trampoline_type trampoline)
{
	lock_guard g2{ global_thread_lock };

	kbl::allocate_checker ck{};
	auto ret = new(&ck) thread{ parent, name };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ret->kstack = kernel_stack::create(ret, routine, arg, trampoline);

	if (ret->kstack == nullptr)
	{
		delete ret;
		return -ERROR_MEMORY_ALLOC;
	}

	if (parent != nullptr)
	{
		if (auto alloc_ret = parent->allocate_ustack(ret);has_error(alloc_ret))
		{
			return get_error_code(alloc_ret);
		}

		ret->kstack->tf->rsp = reinterpret_cast<uintptr_t>(ret->ustack->get_top());
	}

	ret->state = thread_states::READY;

	global_thread_list.push_back(*ret);

	return ret;
}

error_code thread::create_idle()
{
	[[maybe_unused]]auto ret = create(nullptr, "idle", idle_routine, nullptr, default_trampoline);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	auto th = get_result(ret);

	lock_guard g1{ global_thread_lock };

	th->flags |= FLAG_IDLE;

	th->need_reschedule = true;

	cpu->idle = th;
	cur_thread = cpu->idle;

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

//FIXME
extern cls_item<task::process*, CLS_PROC_STRUCT_PTR> cur_proc;

void thread::switch_to() TA_REQ(global_thread_lock)
{
//	kdebug::kdebug_log("[cpu %d]%s->%s", cpu->id, cur_thread->name.data(), this->name.data());

	KDEBUG_ASSERT(this->state == thread_states::READY);

	if (this != cur_thread)
	{
		trap::pushcli();

		auto prev = cur_thread.get();

		{
			lock_guard g{ lock };
			state = thread_states::RUNNING;

			//FIXME
			if (this->parent)
			{
				cur_proc = this->parent;
			}
		}

		uintptr_t kstack_addr = this->kstack->get_address();
		cpu->tss.rsp0 = kstack_addr + kernel_stack::MAX_SIZE;

		// Set gs. without calling swapgs to ensure atomic
		gs_put_cpu_dependent(KERNEL_GS_KSTACK, kstack_addr);

		auto mm = this->get_mm();
		if (mm == nullptr)
		{
			lcr3(V2P((uintptr_t)vmm::g_kpml4t));
		}
		else
		{
			lcr3(V2P((uintptr_t)this->get_mm()->pgdir));
		}

		cur_thread = this;

		trap::popcli();

		context_switch(&prev->kstack->context, this->kstack->context);

	}
}

thread::thread(process* prt,
	ktl::string_view nm)
	: parent{ prt },
	  name{ nm }
{
}

void thread::remove_from_lists()
{
	global_thread_list.erase(thread_list_type::iterator_type{ &this->thread_link });
}

void thread::finish_dead_transition()
{
	{
		this->state = thread_states::DEAD;

		if (critical)
		{
			// TODO: notify the process
			KDEBUG_NOT_IMPLEMENTED;
		}
	}

	delete this;
}

[[noreturn]] error_code thread::idle_routine(void* arg __UNUSED)
{
	for (;;)
	{
		cpu->scheduler.yield();
	}

	__UNREACHABLE; // do not do return -ERROR_SHOULD_NOT_REACH_HERE;
}

void thread::kill()
{
	{
		lock_guard g1{ global_thread_lock };

		signals |= SIGNAL_KILLED;

		if (this == cur_thread)
		{
			return;
		}
	}
	// TODO: wakeup

	cpu->scheduler.yield();
}

void thread::resume()
{

}

void thread::suspend()
{

}

void thread::forget()
{
	{
		lock_guard g{ global_thread_lock };
		KDEBUG_ASSERT(this != cur_thread.get());

		this->remove_from_lists();
	}

	// TODO if waiting?

	delete this;
}

error_code thread::detach()
{
	return 0;
}

error_code thread::join(error_code* out_err_code)
{

	return 0;
}

void thread::current::exit(error_code code)
{
	{
		lock_guard g1{ global_thread_lock };

		if (cur_thread->parent != nullptr)
		{
			cur_thread->parent->remove_thread(cur_thread.get());
		}

		cur_thread->state = thread_states::DYING;
		cur_thread->exit_code = code;

		if (cur_thread->flags & FLAG_DETACHED)
		{
			cur_thread->remove_from_lists();
		}
		else
		{
			// TODO: waiting queue
		}
	}

	cpu->scheduler.reschedule();

	__UNREACHABLE;
}

user_stack::user_stack(process* p, thread* t, void* stack_ptr)
	: top{ stack_ptr }, owner_process{ p }, owner_thread{ t }
{
}

void* user_stack::get_top()
{
	return top;
}
