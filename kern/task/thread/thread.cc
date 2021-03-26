#include "../include/syscall.h"
#include "internals/thread.hpp"

#include "task/thread/thread.hpp"
#include "task/process/process.hpp"
#include "task/scheduler/scheduler.hpp"

#include "system/mmu.h"
#include "system/vmm.h"
#include "system/kmalloc.hpp"
#include "system/scheduler.h"
#include "system/deadline.hpp"

#include "drivers/acpi/cpu.h"

#include "kbl/lock/lock_guard.hpp"

#include <gsl/util>

#include <utility>

using namespace task;

using lock::lock_guard;

lock::spinlock task::global_thread_lock{ "gtl" };

cls_item<thread*, CLS_CUR_THREAD_PTR> task::cur_thread TA_GUARDED(global_thread_lock);

thread::master_list_type task::global_thread_list TA_GUARDED(global_thread_lock);

void task_state::init(thread_routine_type entry, void* arg)
{
	routine_ = entry;
	arg_ = arg;
}

error_code task_state::join(const deadline& ddl) TA_REQ(global_thread_lock)
{
	return exit_code_wait_queue_.block(wait_queue::interruptible::No, ddl);
}

void task_state::wake_joiners(error_code status) TA_REQ(global_thread_lock)
{
	exit_code_wait_queue_.wake_all(false, status);
}

ktl::unique_ptr<kernel_stack> task::kernel_stack::create(thread* parent,
	task::thread_routine_type start_routine,
	void* arg,
	task::thread_trampoline_type tpl)
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

	return ktl::unique_ptr<kernel_stack>(ret);
}

kernel_stack::~kernel_stack()
{
	memory::kfree(bottom);
}

kernel_stack::kernel_stack(thread* parent_thread,
	void* stk_mem,
	task::thread_routine_type routine,
	void* arg,
	task::thread_trampoline_type tpl)
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

	tf->rflags |= EFLAG_IF | EFLAG_IOPL_MASK;

	// argument: routine arg and exit callback
	tf->rdi = reinterpret_cast<uintptr_t>(routine);
	tf->rsi = reinterpret_cast<uintptr_t>(arg);
	tf->rdx = reinterpret_cast<uintptr_t>(thread::current::exit);

	tf->rip = reinterpret_cast<uintptr_t >(thread_entry);

	// setup registers
	if (parent_thread->parent_ != nullptr) // the thread is for user
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
	thread_routine_type routine,
	void* arg,
	thread_trampoline_type trampoline,
	cpu_affinity aff)
{
	kbl::allocate_checker ck{};
	auto ret = new(&ck) thread{ parent, name, aff };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ret->kstack_ = kernel_stack::create(ret, routine, arg, trampoline);

	if (ret->kstack_ == nullptr)
	{
		delete ret;
		return -ERROR_MEMORY_ALLOC;
	}

	if (parent != nullptr)
	{
		if (auto alloc_ret = parent->user_stack_state_.allocate_ustack(ret);has_error(alloc_ret))
		{
			return get_error_code(alloc_ret);
		}

		ret->kstack_->tf->rsp = reinterpret_cast<uintptr_t>(ret->ustack_->get_top());
	}

	ret->state = thread_states::READY;

	{
		lock_guard g2{ global_thread_lock };

		global_thread_list.push_back(ret);
	}

	return ret;
}

error_code thread::create_idle()
{
	[[maybe_unused]]auto ret = create(nullptr, "idle", scheduler::idle, nullptr, default_trampoline,
		cpu_affinity{ cpu->id, cpu_affinity_type::HARD });

	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	auto th = get_result(ret);

	lock_guard g1{ global_thread_lock };

	th->flags_ |= FLAG_IDLE;

	th->need_reschedule_ = true;

	cpu->idle = th;

	cur_thread = cpu->idle;

	return ERROR_SUCCESS;
}

vmm::mm_struct* task::thread::get_mm()
{
	if (parent_ != nullptr)
	{
		return parent_->get_mm();
	}

	return nullptr;
}

void thread::default_trampoline()  TA_REL(global_thread_lock) //TA_NO_THREAD_SAFETY_ANALYSIS
{
	global_thread_lock.unlock();

	// will return to thread_entry
}

void thread::switch_to(interrupt_saved_state_type state_to_restore) TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(this->state == thread_states::READY);

	auto _ = gsl::finally([&state_to_restore]()
	{
	  arch_interrupt_restore(state_to_restore);
	});

	if (this != cur_thread)
	{
		auto prev = cur_thread.get();

		{
			lock_guard g{ lock };
			state = thread_states::RUNNING;

			//FIXME
			if (this->parent_)
			{
				cur_proc = this->parent_;
			}
		}

		uintptr_t kstack_addr = this->kstack_->get_address();
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
			lcr3(V2P((uintptr_t)
				this->get_mm()->pgdir));
		}

		cur_thread = this;

		// manually restore interrupt state
		arch_interrupt_restore(state_to_restore);

		context_switch(&prev->kstack_->context, this->kstack_->context);

	}

	// automatically restore interrupt state here
}

thread::thread(process* prt, ktl::string_view nm, cpu_affinity aff)
	: object::solo_dispatcher<thread, 0>(),
	  name_{ nm },
	  parent_{ prt },
	  affinity_{ aff }
{
	auto this_handle = object::handle_entry::create(name_.data(), this);

	if (prt != nullptr)
	{
		auto local_handle = object::handle_entry::duplicate(this_handle.get());
		parent_->handle_table_.add_handle(std::move(local_handle));
	}

	object::handle_table::get_global_handle_table()->add_handle(std::move(this_handle));
}

thread::~thread()
{
	auto pred = [this](const object::handle_entry& h)
	{
	  auto obj = downcast_dispatcher<thread>(h.object());
	  return obj ? obj->get_name().compare(this->name_.data()) == 0 : false;
	};

	{
		auto global_handle = object::handle_table::get_global_handle_table()->query_handle(pred);

		KDEBUG_ASSERT_MSG(global_handle != nullptr, "~thread: global_handle can't be null");

		auto _ = parent_->handle_table_.remove_handle(global_handle);
	}

	if (parent_)
	{

		auto local_handle = parent_->handle_table_.query_handle(pred);

		KDEBUG_ASSERT_MSG(local_handle != nullptr, "~thread: local_handle can't be null");

		auto _ = parent_->handle_table_.remove_handle(local_handle);

	}
}

void thread::remove_from_lists()
{
	global_thread_list.remove(*this);
}

void thread::finish_dead_transition()
{
	{
		this->state = thread_states::DEAD;

		if (is_user_thread())
		{
			this->parent_->user_stack_state_.free_ustack(this->ustack_);
		}

		if (critical_)
		{
			// TODO: notify the process
			KDEBUG_NOT_IMPLEMENTED;
		}
	}

	delete this;
}

void thread::kill() TA_REQ(!global_thread_lock)
{
	lock_guard g{ global_thread_lock };
	kill_locked();
}

void thread::kill_locked()
{
	canary_.assert();

	{
		signals_ |= SIGNAL_KILLED;

		if (this == cur_thread)
		{
			return;
		}
	}

	bool reschedule_needed = false;

	switch (state)
	{

	case thread_states::INITIAL:
		break;
	case thread_states::READY:
		break;
	case thread_states::RUNNING:
		break;
	case thread_states::SUSPENDED:
	{
		reschedule_needed = scheduler::current::unblock(this);
		break;
	}
	case thread_states::BLOCKED:
	case thread_states::BLOCKED_READ_LOCK:
		wait_queue_state_.unblock_if_interruptible(this, ERROR_INTERNAL_INTR_KILLED);
		break;

	case thread_states::SLEEPING:
		reschedule_needed = wait_queue_state_.unsleep_if_interruptible(this, ERROR_INTERNAL_INTR_KILLED);
		break;
	case thread_states::DYING:
		break;
	case thread_states::DEAD:
		return;

	default:
		KDEBUG_GERNERALPANIC_CODE(ERROR_INVALID);
	}

	if (reschedule_needed)
	{
		scheduler::current::reschedule_locked();
	}
}

void thread::resume()
{
	canary_.assert();

	bool intr_disable = arch_ints_disabled();
	bool resched = false;
	if (intr_disable)
	{
		resched = true;
	}

	{
		lock_guard g{ global_thread_lock };

		if (state == thread_states::DEAD || state == thread_states::DYING)
		{
			return;
		}

		signals_ &= ~SIGNAL_SUSPEND;

		if (state == thread_states::INITIAL || state == thread_states::SUSPENDED)
		{
			bool local_reschedule = scheduler::current::unblock(this);
			if (resched && local_reschedule)
			{
				scheduler::current::reschedule_locked();
			}
		}
	}
}

error_code thread::suspend()
{
	KDEBUG_ASSERT(!is_idle());
	bool local_reschedule = false;

	{
		lock_guard g{ global_thread_lock };

		if (state == thread_states::DEAD)
		{
			return -ERROR_INVALID;
		}

		signals_ |= SIGNAL_KILLED;

		switch (state)
		{
		case thread_states::INITIAL:
		{
			local_reschedule = scheduler::current::unblock(this);
			break;
		}
		case thread_states::READY:
			break;
		case thread_states::RUNNING:
			break;
		case thread_states::SUSPENDED:
			break;
		case thread_states::BLOCKED:
		case thread_states::BLOCKED_READ_LOCK:
			wait_queue_state_.unblock_if_interruptible(this, ERROR_INTERNAL_INTR_RETRY);
			break;
		case thread_states::SLEEPING:
			local_reschedule = wait_queue_state_.unsleep_if_interruptible(this, ERROR_INTERNAL_INTR_RETRY);
			break;
		default:
		case thread_states::DYING:
		case thread_states::DEAD:
			KDEBUG_GERNERALPANIC_CODE(ERROR_INVALID);
		}
	}

	if (need_reschedule_ && local_reschedule)
	{
		scheduler::current::reschedule();
	}

	return ERROR_SUCCESS;
}

void thread::forget()
{
	{
		lock_guard g{ global_thread_lock };
		KDEBUG_ASSERT(this != cur_thread.get());

		this->remove_from_lists();

		KDEBUG_ASSERT(!wait_queue_state_.holding());

	}

	delete this;
}

error_code thread::detach()
{
	canary_.assert();

	lock_guard g{ global_thread_lock };

	task_state_.wake_joiners(ERROR_INVALID);

	if (state == thread_states::DEAD)
	{
		flags_ &= ~FLAG_DETACHED;
		g.unlock();
		return join(nullptr);
	}
	else
	{
		flags_ |= FLAG_DETACHED;
		return ERROR_SUCCESS;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code thread::join(error_code* out_err_code)
{
	canary_.assert();

	return join(out_err_code, deadline::infinite());
}

error_code thread::join(error_code* out_err_code, deadline ddl)
{
	canary_.assert();

	{
		lock_guard g{ global_thread_lock };

		if (flags_ & FLAG_DETACHED)
		{
			return ERROR_INVALID;
		}

		if (state != thread_states::DEAD)
		{
			auto ret = task_state_.join(ddl);
			if (ret != ERROR_SUCCESS)
			{
				return ret;
			}
		}

		canary_.assert();

		KDEBUG_ASSERT(state == thread_states::DYING || state == thread_states::DEAD);
		KDEBUG_ASSERT(!wait_queue_state_.holding());

		if (out_err_code)
		{
			*out_err_code = task_state_.get_exit_code();
		}

		this->remove_from_lists();

	}

	if (zombie_queue_link.is_empty_or_detached()) // not in zombie list
	{
		delete this;
	}

	return ERROR_SUCCESS;
}

error_code thread::detach_and_resume()
{
	auto ret = detach();
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}
	resume();
	return ERROR_SUCCESS;
}

void thread::process_pending_signals()
{
	if (signals_ == 0)[[likely]]
	{
		return;
	}
}

void thread::current::exit(error_code code)
{
	bool intr_disabled = arch_ints_disabled();
	if (!intr_disabled)
	{
		cli();
	}

	{
		lock_guard g1{ global_thread_lock };

		cur_thread->canary_.assert();

		if (cur_thread->parent_ != nullptr)
		{
			cur_thread->parent_->remove_thread(cur_thread.get());
		}

		cur_thread->state = thread_states::DYING;
		cur_thread->task_state_.set_exit_code(code);

		if (cur_thread->flags_ & FLAG_DETACHED)
		{
			cur_thread->remove_from_lists();
		}
		else
		{
			cur_thread->canary_.assert();
			cur_thread->task_state_.wake_joiners(code);
		}
	}

	if (!intr_disabled)
	{
		sti();
	}

	scheduler::current::reschedule();

}
