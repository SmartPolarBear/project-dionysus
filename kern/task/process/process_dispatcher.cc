
#include <task/process_dispatcher.hpp>
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

void new_proc_begin()
{
//	spinlock_release(&proc_list.lock);
	proc_list.lock.unlock();

	// "return" to user_proc_entry
}

kbl::integral_atomic<pid_type> process_dispatcher::pid_counter;

error_code_with_result<ktl::shared_ptr<process_dispatcher>> process_dispatcher::create(const char* name,
	ktl::shared_ptr<job_dispatcher> parent)
{
	ktl::span<char> name_span{ (char*)name, (size_t)strnlen(name, PROC_MAX_NAME_LEN) };

	kbl::allocate_checker ck;

	ktl::shared_ptr<process_dispatcher>
		proc{ new(&ck) process_dispatcher{ name_span, process_dispatcher::pid_counter++, parent, nullptr }};

	lock_guard g1{ proc->lock };

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	proc->kstack = std::make_unique<uint8_t[]>(KERNSTACK_SIZE);
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

error_code process_dispatcher::setup_kernel_stack()
{
	auto raw_stack = this->kstack.get();

	auto sp = reinterpret_cast<uintptr_t>(raw_stack + task::process_dispatcher::KERNSTACK_SIZE);

	sp -= sizeof(*this->tf);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(raw_stack);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = (uintptr_t)user_proc_entry;

	sp -= sizeof(*this->context);
	this->context = reinterpret_cast<decltype(this->context)>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)new_proc_begin;

	return ERROR_SUCCESS;
}

error_code process_dispatcher::setup_registers()
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

task::process_dispatcher::process_dispatcher(std::span<char> name,
	pid_type id,
	ktl::shared_ptr<job_dispatcher> parent,
	ktl::shared_ptr<job_dispatcher> critical_to)
	: parent(std::move(parent)), critical_to(std::move(critical_to))
{
	this->name = ktl::span<char>{ _name_buf, name.size() };

	ktl::copy(name.begin(), name.end(), this->name.begin());

	lock::spinlock_initialize_lock(&messaging_data.lock, this->name.data());
}

error_code process_dispatcher::setup_mm()
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
error_code process_dispatcher::load_binary(uint8_t* bin, size_t binary_size, binary_types type, size_t flags)
{
	return 0;
}
error_code process_dispatcher::terminate(error_code terminate_error)
{
	return 0;
}

error_code process_dispatcher::sleep(sleep_channel_type channel, lock::spinlock_struct* lk)
{
	return 0;
}
error_code process_dispatcher::wakeup(sleep_channel_type channel)
{
	return 0;
}
error_code process_dispatcher::wakeup_no_lock(sleep_channel_type channel)
{
	return 0;
}
error_code process_dispatcher::change_heap_ptr(uintptr_t* heap_ptr)
{
	return 0;
}

void process_dispatcher::finish_dead_transition() noexcept
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

	set_status_locked(Status::DEAD);

	parent->remove_child_process(this);

	ktl::shared_ptr<job_dispatcher> kill_job{ nullptr };
	{
		lock_guard guard{ lock };

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
		kill_job->kill(TASK_RETCODE_CRITICAL_PROCESS_KILL);
	}
}

void process_dispatcher::exit(task_return_code code) noexcept
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
	}

	// TODO: Exit all threads


	KDEBUG_NOT_IMPLEMENTED;

	__UNREACHABLE;
}
void process_dispatcher::kill(task_return_code code) noexcept
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

void process_dispatcher::set_status_locked(process_dispatcher::Status st) noexcept TA_REQ(lock)
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

void process_dispatcher::kill_all_threads_locked() noexcept TA_REQ(lock)
{
	KDEBUG_NOT_IMPLEMENTED;
}
