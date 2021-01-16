#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

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

using kbl::list_add;
using kbl::list_empty;
using kbl::list_for_each;
using kbl::list_init;
using kbl::list_remove;

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

using namespace task;

using namespace memory;
using namespace vmm;

cls_item<process*, CLS_PROC_STRUCT_PTR> cur_proc;

process_list_struct proc_list;

std::shared_ptr<job> root_job;


//static inline FIXME
error_code task::alloc_ustack(task::process* proc)
{
	auto ret = vmm::mm_map(proc->mm,
		USER_TOP - task::process::KERNSTACK_SIZE - 1,
		task::process::KERNSTACK_PAGES * PAGE_SIZE,
		vmm::VM_STACK, nullptr);
	if (ret != ERROR_SUCCESS)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	// allocate an stack
	for (size_t i = 0;
		 i < task::process::KERNSTACK_PAGES;
		 i++)
	{
		uintptr_t va = USER_TOP - task::process::KERNSTACK_SIZE + i * PAGE_SIZE;
		page_info* page_ret = nullptr;

		ret = pmm::pgdir_alloc_page(proc->mm->pgdir,
			true,
			va,
			PG_W | PG_U | PG_PS | PG_P,
			&page_ret);

		if (ret != ERROR_SUCCESS)
		{
			return -ERROR_MEMORY_ALLOC;
		}
	}

	return ERROR_SUCCESS;
}

//static inline FIXME
size_t task::process_terminate_impl(task::process* proc,
	error_code err)
{
	if ((proc->flags & task::PROC_EXITING) == 0)
	{
		proc->flags |= task::PROC_EXITING;
		proc->exit_code = err;

//		if (task->wating_state & task::PROC_WAITING_INTERRUPTED)
//		{
//
//		}
		if (proc->state == task::PROC_STATE_SLEEPING)
		{
			proc->state = task::PROC_STATE_RUNNABLE;
		}

		return ERROR_SUCCESS;
	}
	return -ERROR_HAS_KILLED;
}

task::process* find_process(pid_type pid)
{
//	list_head* iter = nullptr;
//	list_for(iter, &proc_list.active_head)
//	{
//		auto proc_item = container_of(iter, task::process, link);
//
//		if (proc_item->id == pid)
//		{
//			return proc_item;
//		}
//	}

	decltype(&proc_list.head) iter;
	llb_for(iter, &proc_list.head)
	{
		auto iter_proc = iter->get_element_as<process*>();
		if (iter_proc->get_id() == pid)
		{
			return (task::process*)iter;
		}
	}

	return nullptr;
}

void task::process_init(void)
{
	proc_list.proc_count = 0;

	auto create_ret = task::job::create_root();

	if (has_error(create_ret))
	{
		KDEBUG_GERNERALPANIC_CODE(get_error_code(create_ret));
	}
	else
	{
		root_job = get_result(create_ret);
	}

//	spinlock_initialize_lock(&proc_list.lock, "task");
//	list_init(&proc_list.active_head);
}

error_code task::process_load_binary(IN process* proc,
	IN uint8_t* bin,
	[[maybe_unused]] IN size_t binary_size OPTIONAL,
	IN binary_types type,
	IN size_t flags
)
{
//	spinlock_acquire(&proc_list.lock);

	ktl::mutex::lock_guard guard{ proc_list.lock };
	error_code ret = ERROR_SUCCESS;

	uintptr_t entry_addr = 0;
	if (type == BINARY_ELF)
	{
		ret = load_binary(proc, bin, binary_size, &entry_addr);
	}
	else
	{
		ret = -ERROR_INVALID;
	}

	if (ret == ERROR_SUCCESS)
	{
		ret = alloc_ustack(proc);

		if (ret == ERROR_SUCCESS)
		{
			proc->tf->rip = entry_addr;

			if (flags & LOAD_BINARY_RUN_IMMEDIATELY)
			{
				proc->state = PROC_STATE_RUNNABLE;
			}
		}
		else
		{
			return ret;
		}
	}

//	spinlock_release(&proc_list.lock);

	return ret;
}

error_code task::process_terminate(error_code err)
{
	return process_terminate_impl(cur_proc(), err);
}

void task::process_exit(IN process* proc)
{
//	spinlock_acquire(&proc_list.lock);
	proc_list.lock.lock();

	// TODO: close all files

	// TODO: wakeup parent and inform it of the termination

	// TODO: recycle the memory

	auto mm = proc->mm;
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

	proc->mm = nullptr;

	// set task state and call the context
	proc->state = PROC_STATE_ZOMBIE;

	proc_list.zombie_queue.push(proc);

	::scheduler::scheduler_enter();
}

// let current task sleep on certain channel
error_code task::process_sleep(size_t channel, lock::spinlock_struct* lock)
{
	if (cur_proc == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (lock == nullptr)
	{
		return -ERROR_INVALID;
	}

	// we always only hold proc_list.lock no matter what the lock is
	// FIXME
	KDEBUG_NOT_IMPLEMENTED;
//	if (lock != &proc_list.lock)
//	{
//		spinlock_acquire(&proc_list.lock);
//		spinlock_release(lock);
//	}

	cur_proc->sleep_data.channel = channel;
	cur_proc->state = PROC_STATE_SLEEPING;

	::scheduler::scheduler_enter();

	cur_proc->sleep_data.channel = 0;

	// restore the lock
	// FIXME
	KDEBUG_NOT_IMPLEMENTED;
//	if (lock != &proc_list.lock)
//	{
//		spinlock_release(&proc_list.lock);
//		spinlock_acquire(lock);
//	}

	return ERROR_SUCCESS;
}

// wake up child_processes sleeping on certain channel
error_code task::process_wakeup(size_t channel)
{
//	spinlock_acquire(&proc_list.lock);
	ktl::mutex::lock_guard guard{ proc_list.lock };
	auto ret = process_wakeup_nolock(channel);
//	spinlock_release(&proc_list.lock);
	return ret;
}

error_code task::process_wakeup_nolock(size_t channel)
{
//	list_head* iter = nullptr;
//	list_for(iter, &proc_list.active_head)
//	{
//		auto iter_proc = list_entry(iter, task::process, link);
//		if (iter_proc->state == PROC_STATE_SLEEPING && iter_proc->sleep_data.channel == channel)
//		{
//			iter_proc->state = PROC_STATE_RUNNABLE;
//		}
//	}

	decltype(&proc_list.head) iter;
	llb_for(iter, &proc_list.head)
	{
		auto iter_proc = iter->get_element_as<process*>();
		if (iter_proc->state == PROC_STATE_SLEEPING && iter_proc->sleep_data.channel == channel)
		{
			iter_proc->state = PROC_STATE_RUNNABLE;
		}
	}

	return ERROR_SUCCESS;
}

error_code task::process_heap_change_size(IN process* proc, IN OUT uintptr_t* heap_ptr)
{
	auto mm = proc->mm;

	if (mm == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (!VALID_USER_REGION((uintptr_t)heap_ptr, ((uintptr_t)heap_ptr) + sizeof(uintptr_t)))
	{
		return -ERROR_INVALID;
	}

	ktl::mutex::lock_guard guard{ proc_list.lock };

//	spinlock_acquire(&mm->lock);

	uintptr_t heap = 0;
	memmove(&heap, heap_ptr, sizeof(heap));

	if (heap < mm->brk_start)
	{
		*heap_ptr = mm->brk_start;
	}
	else
	{
		uintptr_t new_heap = PAGE_ROUNDUP(heap), old_heap = mm->brk;

		if ((old_heap % PAGE_SIZE) != 0)
		{
			return -ERROR_INVALID;
		}

		if (new_heap == old_heap)
		{
			*heap_ptr = mm->brk_start;
		}
		else if (new_heap < old_heap) // shrink
		{
			if (mm_unmap(mm, new_heap, old_heap - new_heap) != ERROR_SUCCESS)
			{
				*heap_ptr = mm->brk_start;
			}
		}
		else if (new_heap > old_heap) // expand
		{
			if (mm_intersect_vma(mm, old_heap, new_heap + PAGE_SIZE) != nullptr)
			{
				*heap_ptr = mm->brk_start;
			}
			else
			{
				if (mm_change_size(mm, old_heap, (size_t)new_heap - old_heap) != ERROR_SUCCESS)
				{
					*heap_ptr = mm->brk_start;
				}
			}
		}
	}

//	spinlock_release(&mm->lock);

	return ERROR_SUCCESS;
}

