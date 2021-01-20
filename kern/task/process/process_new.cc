
#include <task/process/process.hpp>
#include <utility>

#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

#include "compiler/compiler_extensions.hpp"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"

#include "kbl/lock/spinlock.h"
#include "kbl/data/pod_list.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/algorithm.hpp"
#include "ktl/shared_ptr.hpp"

using namespace kbl;
using namespace lock;
using namespace memory;
using namespace vmm;
using namespace ktl::mutex;
using namespace task;

using namespace task;

void default_trampoline()
{
//	spinlock_release(&proc_list.lock);
	proc_list.lock.unlock();

	// "return" to user_entry
}

kbl::integral_atomic<pid_type> process::pid_counter;

error_code_with_result<ktl::shared_ptr<process>> process::create(const char* name,
	ktl::shared_ptr<job> parent)
{
	ktl::span<char> name_span{ (char*)name, (size_t)strnlen(name, PROC_MAX_NAME_LEN) };

	kbl::allocate_checker ck;

	ktl::shared_ptr<process>
		proc{ new(&ck) process{ name_span, process::pid_counter++, parent, nullptr }};

	lock_guard g1{ proc->lock };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	proc->kstack = std::make_unique<uint8_t[]>(KSTACK_SIZE);

	if (auto ret = proc->setup_kernel_stack();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = proc->setup_mm();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = proc->setup_registers();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	parent->add_child_process(proc);

	// FIXME process should no longer be minimal execution unit
	ktl::mutex::lock_guard guard{ proc_list.lock };
	proc_list.head.insert_after(proc.get());
	return proc;
}

error_code process::setup_kernel_stack()
{
	auto raw_stack = this->kstack.get();

	auto sp = reinterpret_cast<uintptr_t>(raw_stack + task::process::KSTACK_SIZE);

	sp -= sizeof(*this->tf);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(raw_stack);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = (uintptr_t)user_entry;

	sp -= sizeof(*this->context);
	this->context = reinterpret_cast<decltype(this->context)>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)default_trampoline;

	return ERROR_SUCCESS;
}

error_code process::setup_registers()
{
	tf->cs = SEGMENT_VAL(SEGMENTSEL_UCODE, DPL_USER);
	tf->ss = SEGMENT_VAL(SEGMENTSEL_UDATA, DPL_USER);
	tf->rsp = USER_STACK_TOP;
	tf->rflags |= trap::EFLAG_IF;

	if ((flags & PROC_SYS_SERVER) || (flags & PROC_DRIVER))
	{
		tf->rflags |= trap::EFLAG_IOPL_MASK;
	}

	return ERROR_SUCCESS;
}

task::process::process(std::span<char> name,
	pid_type id,
	ktl::shared_ptr<job> parent,
	ktl::shared_ptr<job> critical_to)
	: parent(std::move(parent)), critical_to(std::move(critical_to))
{
	this->name = ktl::span<char>{ _name_buf, name.size() };

	ktl::copy(name.begin(), name.end(), this->name.begin());

	lock::spinlock_initialize_lock(&messaging_data.lock, this->name.data());
}

error_code process::setup_mm()
{
	this->mm = vmm::mm_create();
	if (this->mm == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	vmm::pde_ptr_t pgdir = vmm::pgdir_entry_alloc();

	if (pgdir == nullptr)
	{
		vmm::mm_destroy(this->mm);
		return -ERROR_MEMORY_ALLOC;
	}

	vmm::duplicate_kernel_pml4t(pgdir);

	this->mm->pgdir = pgdir;

	return ERROR_SUCCESS;
}

void process::finish_dead_transition() noexcept
{
	if (mm != nullptr)
	{
		if ((--mm->map_count) == 0)
		{
			// restore to kernel page table
			vmm::install_kernel_pml4t();

			// free memory
			vmm::mm_free(mm);

			vmm::pgdir_entry_free(mm->pgdir);

			trap::pushcli();

			trap::popcli();

			vmm::mm_destroy(mm);
		}
	}

	mm = nullptr;

	parent->remove_child_process(this);

	ktl::shared_ptr<job> kill_job{ nullptr };

	{
		lock_guard guard{ lock };

		set_status_locked(Status::DEAD);

		if (critical_to != nullptr)
		{
			if (!kill_critical_when_nonzero_code || ret_code != 0)
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
		ktl::mutex::lock_guard guard{ this->lock };

		if (this->status == Status::DYING)
		{
			return;
		}

		this->ret_code = code;

		set_status_locked(Status::DYING);

		kill_all_threads_locked();
	}

	__UNREACHABLE;
}
void process::kill(task_return_code code) noexcept
{
	bool finish_dying = false;

	{
		lock_guard guard{ lock };

		if (status == Status::DEAD)
		{
			return;
		}

		if (status != Status::DYING)
		{
			ret_code = code;
		}

		if (threads.empty())
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

void process::set_status_locked(process::Status st) noexcept TA_REQ(lock)
{
	KDEBUG_ASSERT(lock.holding());

	if (status == Status::DEAD && st != Status::DEAD)
	{
		KDEBUG_GENERALPANIC("Bad transition from dead to other status.");
		return;
	}

	if (status == st)
	{
		return;
	}

	status = st;
	if (st == Status::DYING)
	{
		kill_all_threads_locked();
	}
}

void process::kill_all_threads_locked() noexcept TA_REQ(lock)
{
	lock_guard g{ global_thread_lock };

	for (auto& t:threads)
	{
		t->kill();
	}
}

void task::process::remove_thread(task::thread* t)
{
	lock_guard g{ lock };

	this->threads.remove(t);
}

error_code_with_result<void*> task::process::make_next_user_stack()
{
	const uintptr_t current_top = USTACK_TOTAL_SIZE * busy_list.size();

	auto ret = vmm::mm_map(this->mm,
		current_top - process::USTACK_TOTAL_SIZE - 1, //TODO -1?
		process::USTACK_TOTAL_SIZE,
		vmm::VM_STACK, nullptr);

	if (ret != ERROR_SUCCESS)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	// allocate an stack
	for (size_t i = 0;
	     i < task::process::USTACK_PAGES_PER_THREAD; // do not allocate the guard page
	     i++)
	{
		uintptr_t va = current_top - process::USTACK_USABLE_SIZE_PER_THREAD + i * PAGE_SIZE;
		page_info* page_ret = nullptr;

		ret = pmm::pgdir_alloc_page(this->mm->pgdir,
			true,
			va,
			PG_W | PG_U | PG_PS | PG_P,
			&page_ret);

		if (ret != ERROR_SUCCESS)
		{
			return -ERROR_MEMORY_ALLOC;
		}
	}

	[[maybe_unused]]page_info* guard_page = nullptr;

	ret = pmm::pgdir_alloc_page(this->mm->pgdir,
		true,
		current_top - process::USTACK_TOTAL_PAGES_PER_THREAD,
		PG_U | PG_PS | PG_P, // guard page can't be written
		&guard_page);

	if (ret != ERROR_SUCCESS)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	return (void*)(current_top - process::USTACK_USABLE_SIZE_PER_THREAD);
}

error_code_with_result<user_stack*> task::process::allocate_ustack(thread* t)
{
	lock_guard g{ lock };

	if (!free_list.empty())
	{
		auto stack = free_list.front();
		free_list.pop_front();

		{
			auto iter = busy_list.begin();
			while (*(*iter) < *stack); // forward the iterator until iter is greater or equal stack
			busy_list.insert(--iter, stack);
		}

		return stack;
	}

	kbl::allocate_checker ac{};

	user_stack* stack = nullptr;
	if (auto alloc_ret = make_next_user_stack();has_error(alloc_ret))
	{
		return get_error_code(alloc_ret);
	}
	else
	{
		stack = new(&ac) user_stack{ this, t, get_result(alloc_ret) };
	}

	if (!ac.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	busy_list.push_back(stack);

	t->ustack = stack;

	return stack;
}

void task::process::free_ustack(task::user_stack* ustack)
{
	lock_guard g{ lock };

	if (free_list.size() >= USTACK_FREELIST_THRESHOLD)
	{
		auto back = free_list.back();
		free_list.pop_back();

		delete back;
	}

	auto iter = free_list.begin();
	while (*(*iter) < *ustack); // forward the iterator until iter is greater or equal stack
	free_list.insert(--iter, ustack);
}

error_code_with_result<ktl::shared_ptr<task::process>> task::process::create(const char* name,
	void* bin,
	size_t size,
	ktl::shared_ptr<job> parent)
{
	ktl::shared_ptr<process> proc{ nullptr };
	if (auto ret = task::process::create(name, std::move(parent));has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		proc = get_result(ret);
	}

	lock_guard g{ proc->lock };

	// TODO: write better loader
	uintptr_t entry_addr = 0;
	if (auto ret = load_binary(proc.get(), (uint8_t*)bin, size, &entry_addr);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	thread* main_thread = nullptr;
	if (auto ret = thread::create(proc.get(), "main", (thread::routine_type)entry_addr, nullptr);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		main_thread = get_result(ret);
	}
	
	// the thread is critical to the process
	main_thread->critical = true;

	proc->threads.push_back(main_thread);

	return proc;
}
