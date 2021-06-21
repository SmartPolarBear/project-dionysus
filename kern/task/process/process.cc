#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

#include "compiler/compiler_extensions.hpp"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "memory/pmm.hpp"

#include "drivers/apic/traps.h"

#include "kbl/lock/spinlock.h"
#include "kbl/data/pod_list.h"
#include "kbl/checker/allocate_checker.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/algorithm.hpp"
#include "ktl/shared_ptr.hpp"

#include "task/process/process.hpp"
#include "task/job/job.hpp"
#include "task/process/process.hpp"

#include <utility>

using namespace kbl;
using namespace lock;
using namespace memory;
using namespace vmm;
using namespace task;

using namespace task;

cls_item<process*, CLS_PROC_STRUCT_PTR> cur_proc;

std::shared_ptr<job> root_job;

error_code_with_result<void*> task::process_user_stack_state::make_next_user_stack_locked()
{
	const uintptr_t current_top = USER_STACK_TOP - USTACK_TOTAL_SIZE * busy_list_.size();

	auto as = parent_->address_space();
	KDEBUG_ASSERT(as != nullptr);

	auto ret = as->map(current_top - USTACK_TOTAL_SIZE - 1, //TODO -1?
		USTACK_TOTAL_SIZE,
		VM_STACK);

	if (has_error(ret))
	{
		return get_error_code(ret);
	}

//	auto ret = vmm::mm_map(parent_->mm,
//		current_top - USTACK_TOTAL_SIZE - 1, //TODO -1?
//		USTACK_TOTAL_SIZE,
//		vmm::VM_STACK, nullptr);
//
//	if (ret != ERROR_SUCCESS)
//	{
//		return -ERROR_MEMORY_ALLOC;
//	}

	// allocate an stack
	for (size_t i = 0;
	     i < USTACK_PAGES_PER_THREAD; // do not allocate the guard page
	     i++)
	{
		uintptr_t va = current_top - USTACK_USABLE_SIZE_PER_THREAD + i * PAGE_SIZE;

		auto alloc_ret = memory::physical_memory_manager::instance()->allocate(va,
			PG_W | PG_U | PG_PS | PG_P,
			as->pgdir(),
			true);

		if (has_error(alloc_ret))
		{
			return get_error_code(alloc_ret);
		}
	}

	auto guard_page_ret =
		memory::physical_memory_manager::instance()->allocate(current_top - USTACK_TOTAL_PAGES_PER_THREAD,
			PG_U | PG_PS | PG_P,
			as->pgdir(),
			true);

	if (has_error(guard_page_ret))
	{
		return get_error_code(guard_page_ret);
	}

	return (void*)(current_top - USTACK_USABLE_SIZE_PER_THREAD);
}

error_code_with_result<user_stack*> task::process_user_stack_state::allocate_ustack(thread* t)
{
	lock_guard g{ lock_ };

	if (!free_list_.empty())
	{
		auto stack = free_list_.front_ptr();
		free_list_.pop_front();

		{
			auto iter = free_list_.begin();
			while (*iter < *stack); // forward the iterator until iter is greater or equal stack
			free_list_.insert(--iter, stack);
		}

		return stack;
	}

	kbl::allocate_checker ac{};

	user_stack* stack = nullptr;
	if (auto alloc_ret = make_next_user_stack_locked();has_error(alloc_ret))
	{
		return get_error_code(alloc_ret);
	}
	else
	{
		stack = new(&ac) user_stack{ parent_, t, get_result(alloc_ret) };
	}

	if (!ac.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	busy_list_.push_back(stack);

	{
		t->ustack_ = stack;
	}

	return stack;
}

void task::process_user_stack_state::free_ustack(task::user_stack* ustack)
{
	lock_guard g{ lock_ };

	if (free_list_.size() >= USTACK_FREELIST_THRESHOLD)
	{
		auto back = free_list_.back_ptr();
		free_list_.pop_back();

		delete back;
	}

	auto iter = free_list_.begin();
	while (*iter < *ustack); // forward the iterator until iter is greater or equal stack
	free_list_.insert(--iter, ustack);
}

task::process_user_stack_state::~process_user_stack_state()
{
	KDEBUG_ASSERT(busy_list_.empty());

	while (!free_list_.empty())
	{
		auto back = free_list_.back_ptr();
		free_list_.pop_back();

		delete back;
	}
}

void task::process_init()
{
	auto create_ret = task::job::create_root();

	if (has_error(create_ret))
	{
		KDEBUG_GERNERALPANIC_CODE(get_error_code(create_ret));
	}
	else
	{
		root_job = get_result(create_ret);
	}

	for (auto& cpu:valid_cpus)
	{
		allocate_checker ck{};
		cpu.scheduler = new(&ck) scheduler{ &cpu };

		if (!ck.check())
		{
			KDEBUG_GERNERALPANIC_CODE(ERROR_MEMORY_ALLOC);
		}
	}
}

error_code_with_result<process*> process::create(const char* name,
	const ktl::shared_ptr<job>& parent)
{
	ktl::span<char> name_span{ (char*)name, (size_t)strnlen(name, PROC_MAX_NAME_LEN) };

	kbl::allocate_checker ck;
	auto proc = new(&ck) process{ name_span, parent, nullptr };

	lock_guard g1{ proc->lock_ };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	if (auto ret = proc->setup_address_space();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	parent->add_child_process(proc);

	return proc;
}

error_code_with_result<process*> task::process::create(const char* name,
	void* bin,
	size_t size,
	const ktl::shared_ptr<job>& parent)
{

	process* proc{ nullptr };
	if (auto ret = task::process::create(name, parent);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		proc = get_result(ret);
	}

	// TODO: write better loader
	uintptr_t entry_addr = 0;
	if (auto ret = load_binary(proc, (uint8_t*)bin, size, &entry_addr);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	thread* main_thread = nullptr;

	// name the main thread with the parent_'s name
	if (auto ret = thread::create(proc, name, (task::thread_routine_type)entry_addr, nullptr);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		main_thread = get_result(ret);
	}

	// the thread is critical_ to the process
	main_thread->critical_ = true;

	{
		lock::lock_guard g{ task::global_thread_lock };
		scheduler::current::unblock(main_thread);
	}

	proc->add_child_thread(main_thread);

	return proc;
}

task::process::process(std::span<char> name,
	const ktl::shared_ptr<job>& parent,
	const ktl::shared_ptr<job>& critical_to)
	: object::solo_dispatcher<process, 0>(),
	  parent_(parent),
	  critical_to_(critical_to)
{
	{
		allocate_checker ck{};
		auto mm = new(&ck)memory::address_space{};

		if (!ck.check())
		{
			KDEBUG_GENERALPANIC(-ERROR_MEMORY_ALLOC);
		}

		auto mm_handle = object::handle_entry::create("mm", mm);
		auto local_mm_handle = object::handle_entry::duplicate(mm_handle.get());

		object::handle_table::get_global_handle_table()->add_handle(std::move(mm_handle));
		address_space_handle_ = handle_table_.add_handle(std::move(local_mm_handle));
	}

	{
		auto this_handle = object::handle_entry::create(name_.data(), this);
		auto local_handle = object::handle_entry::duplicate(this_handle.get());

		object::handle_table::get_global_handle_table()->add_handle(std::move(this_handle));
		this_handle_ = handle_table_.add_handle(std::move(local_handle));
	}

	this->name_.set(name);
}

process::~process()
{
	this->handle_table_.clear();
}

error_code process::setup_address_space()
{
	return address_space()->initialize();
}

void process::finish_dead_transition() noexcept
{

	{
		auto parent_ptr = parent_.lock();
		parent_ptr->remove_child_process(this);
	}

	ktl::shared_ptr<job> kill_job{ nullptr };

	{
		lock_guard guard{ lock_ };

		set_status_locked(Status::DEAD);

		auto critical_to = critical_to_.lock();
		if (critical_to != nullptr)
		{
			if (!kill_critical_when_nonzero_code_ || ret_code_ != 0)
			{
				kill_job = critical_to;
			}
		}
	}

	if (kill_job)
	{
		[[maybe_unused]]auto ret = kill_job->kill(TASK_RETCODE_CRITICAL_PROCESS_KILL);
	}
}

void process::exit(task_return_code code) noexcept
{
	KDEBUG_ASSERT(cur_proc == this);

	{
		lock::lock_guard guard{ this->lock_ };

		if (this->status_ == Status::DYING)
		{
			return;
		}

		this->ret_code_ = code;

		set_status_locked(Status::DYING);

		global_thread_lock.assert_not_held();
		kill_all_threads_locked();
	}

	cur_thread->kill();

	scheduler::current::reschedule();

	KDEBUG_ASSERT(-ERROR_SHOULD_NOT_REACH_HERE);
}

void process::kill(task_return_code code) noexcept
{
	bool finish_dying = false;

	{
		lock_guard guard{ lock_ };

		if (status_ == Status::DEAD)
		{
			return;
		}

		if (status_ != Status::DYING)
		{
			ret_code_ = code;
		}

		if (threads_.empty())
		{
			set_status_locked(Status::DEAD);
			finish_dying = true;
		}
		else
		{
			set_status_locked(Status::DYING);

		}
	}

	if (finish_dying)
	{
		finish_dead_transition();
	}
}

void process::set_status_locked(process::Status st) noexcept TA_REQ(lock_)
{
	KDEBUG_ASSERT(lock_.holding());

	if (status_ == Status::DEAD && st != Status::DEAD)
	{
		KDEBUG_GENERALPANIC("Bad transition from dead to other status.");
		return;
	}

	if (status_ == st)
	{
		return;
	}

	status_ = st;
	if (st == Status::DYING)
	{
		global_thread_lock.assert_not_held();
		kill_all_threads_locked();
	}
}

void process::kill_all_threads_locked() noexcept
{
	for (auto& t:threads_)
	{
		t.kill();
	}
}

void task::process::remove_thread(task::thread* t)
{
	lock_guard g{ lock_ };

	threads_.remove(t);
}

void task::process::add_child_thread(thread* t) noexcept
{
	lock_guard g{ lock_ };
	threads_.push_back(t);
}

error_code task::process::resize_heap(IN OUT uintptr_t* heap_ptr)
{
	lock_guard g1{ lock_ };

	// FIXME
//	lock_guard g2{ mm->lock };

//	if (mm == nullptr)
//	{
//		return -ERROR_INVALID;
//	}

	if (!VALID_USER_REGION((uintptr_t)heap_ptr, ((uintptr_t)heap_ptr) + sizeof(uintptr_t)))
	{
		return -ERROR_INVALID;
	}

	uintptr_t heap = 0;
	memmove(&heap, heap_ptr, sizeof(heap));

	auto as = address_space();

	if (heap < as->heap_begin())
	{
		*heap_ptr = as->heap_begin();
	}
	else
	{
		uintptr_t new_heap = PAGE_ROUNDUP(heap), old_heap = as->heap();

		if ((old_heap % PAGE_SIZE) != 0)
		{
			return -ERROR_INVALID;
		}

		if (new_heap == old_heap)
		{
			*heap_ptr = as->heap_begin();
		}
		else if (new_heap < old_heap) // shrink
		{
//			if (mm_unmap(mm, new_heap, old_heap - new_heap) != ERROR_SUCCESS)
//			{
//				*heap_ptr = mm->brk_start;
//			}
			if (as->unmap(new_heap, old_heap - new_heap) != ERROR_SUCCESS)
			{
				*heap_ptr = as->heap_begin();
			}
		}
		else if (new_heap > old_heap) // expand
		{
//			if (mm_intersect_vma(mm, old_heap, new_heap + PAGE_SIZE) != nullptr)
//			{
//				*heap_ptr = mm->brk_start;
//			}
//			else
//			{
//				if (mm_change_size(mm, old_heap, (size_t)new_heap - old_heap) != ERROR_SUCCESS)
//				{
//					*heap_ptr = mm->brk_start;
//				}
//			}
			if (as->intersect_vma(old_heap, new_heap + PAGE_SIZE) != nullptr)
			{
				*heap_ptr = as->heap_begin();
			}
			else
			{
				if (address_space()->resize(old_heap, (size_t)new_heap - old_heap) != ERROR_SUCCESS)
				{
					*heap_ptr = as->heap_begin();
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

error_code process::suspend()
{
	canary_.assert();

	lock_guard g{ lock_ };

	if (status_ == Status::DYING || status_ == Status::DEAD)
	{
		return -ERROR_INVALID;
	}

	KDEBUG_ASSERT(suspend_count_ >= 0);
	++suspend_count_;
	if (suspend_count_ == 1)
	{
		for (auto& thread:threads_)
		{
			error_code err = thread.suspend();
			KDEBUG_ASSERT(err == ERROR_SUCCESS
				|| (thread.state == thread::thread_states::DYING || thread.state == thread::thread_states::DEAD));
		}
	}

	return ERROR_SUCCESS;
}

void process::resume()
{
	canary_.assert();

	lock_guard g{ lock_ };

	if (status_ == Status::DYING || status_ == Status::DEAD)
	{
		return;
	}

	KDEBUG_ASSERT(suspend_count_ > 0);
	--suspend_count_;
	if (suspend_count_ == 0)
	{
		for (auto& thread:threads_)
		{
			thread.resume();
		}
	}
}

address_space* process::address_space()
{
	// it really can be nullptr when it's wrongly called
	KDEBUG_ASSERT(this != nullptr);

	if (address_space_handle_ == object::INVALID_HANDLE_VALUE)
	{
		return nullptr;
	}

	auto handle = handle_table_.get_handle_entry(address_space_handle_);
	return downcast_dispatcher<memory::address_space>(handle->object());
}

