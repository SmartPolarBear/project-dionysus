#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu.h"
#include "arch/amd64/msr.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "kbl/pod_list.h"
#include "../../libs/basic_io/include/builtin_text_io.hpp"

using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

using namespace task;

using namespace memory;
using namespace vmm;

CLSItem<process_dispatcher*, CLS_PROC_STRUCT_PTR> cur_proc;

process_list_struct proc_list;

void new_proc_begin()
{
	spinlock_release(&proc_list.lock);

	// "return" to user_proc_entry
}

// precondition: the lock must be held
static inline process_id alloc_pid(void)
{
	return proc_list.proc_count++;
}

static inline error_code alloc_ustack(process::process_dispatcher* proc)
{
	auto ret = vmm::mm_map(proc->mm,
		USER_TOP - process::process_dispatcher::KERNSTACK_SIZE - 1,
		process::process_dispatcher::KERNSTACK_PAGES * PAGE_SIZE,
		vmm::VM_STACK, nullptr);
	if (ret != ERROR_SUCCESS)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	// allocate an stack
	for (size_t i = 0;
		 i < process::process_dispatcher::KERNSTACK_PAGES;
		 i++)
	{
		uintptr_t va = USER_TOP - process::process_dispatcher::KERNSTACK_SIZE + i * PAGE_SIZE;
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

static inline error_code setup_mm(process::process_dispatcher* proc)
{
	proc->mm = vmm::mm_create();
	if (proc->mm == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	vmm::pde_ptr_t pgdir = vmm::pgdir_entry_alloc();

	if (pgdir == nullptr)
	{
		vmm::mm_destroy(proc->mm);
		return -ERROR_MEMORY_ALLOC;
	}

	vmm::duplicate_kernel_pml4t(pgdir);

	proc->mm->pgdir = pgdir;

	return ERROR_SUCCESS;
}

static inline size_t process_terminate_impl(process::process_dispatcher* proc,
	error_code err)
{
	if ((proc->flags & process::PROC_EXITING) == 0)
	{
		proc->flags |= process::PROC_EXITING;
		proc->exit_code = err;

//		if (task->wating_state & process::PROC_WAITING_INTERRUPTED)
//		{
//
//		}
		if (proc->state == process::PROC_STATE_SLEEPING)
		{
			proc->state = process::PROC_STATE_RUNNABLE;
		}

		return ERROR_SUCCESS;
	}
	return -ERROR_HAS_KILLED;
}

process::process_dispatcher* find_process(process_id pid)
{
	list_head* iter = nullptr;
	list_for(iter, &proc_list.active_head)
	{
		auto proc_item = container_of(iter, process::process_dispatcher, link);

		if (proc_item->id == pid)
		{
			return proc_item;
		}
	}

	return nullptr;
}

void process::process_init(void)
{
	proc_list.proc_count = 0;
	spinlock_initialize_lock(&proc_list.lock, "task");
	list_init(&proc_list.active_head);
}

error_code process::create_process(IN const char* name,
	IN size_t flags,
	IN bool inherit_parent,
	OUT process_dispatcher** retproc)
{
	spinlock_acquire(&proc_list.lock);

	if (proc_list.proc_count >= process::PROC_MAX_COUNT)
	{
		return -ERROR_TOO_MANY_PROC;
	}

	auto proc = new(std::nothrow)process_dispatcher({ (char*)name, std::dynamic_extent },
		alloc_pid(),
		cur_proc == nullptr ? 0 : cur_proc->get_id(),
		flags);
	if (proc == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	if (inherit_parent)
	{
		// TODO: copy kbl from parent process
	}

	list_add(&proc->link, &proc_list.active_head);

	spinlock_release(&proc_list.lock);

	*retproc = proc;
	return ERROR_SUCCESS;
}

error_code process::process_load_binary(IN process_dispatcher* proc,
	IN uint8_t* bin,
	[[maybe_unused]] IN size_t binary_size OPTIONAL,
	IN binary_types type,
	IN size_t flags
)
{
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

	return ret;
}

error_code process::process_terminate(error_code err)
{
	return process_terminate_impl(cur_proc(), err);
}

void process::process_exit(IN process_dispatcher* proc)
{
	spinlock_acquire(&proc_list.lock);

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

	// set process state and call the scheduler
	proc->state = PROC_STATE_ZOMBIE;

	proc_list.zombie_queue.push(proc);

	scheduler::scheduler_enter();
}

// let current process sleep on certain channel
error_code process::process_sleep(size_t channel, lock::spinlock* lock)
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
	if (lock != &proc_list.lock)
	{
		spinlock_acquire(&proc_list.lock);
		spinlock_release(lock);
	}

	cur_proc->sleep_data.channel = channel;
	cur_proc->state = PROC_STATE_SLEEPING;

	scheduler::scheduler_enter();

	cur_proc->sleep_data.channel = 0;

	// restore the lock
	if (lock != &proc_list.lock)
	{
		spinlock_release(&proc_list.lock);
		spinlock_acquire(lock);
	}

	return ERROR_SUCCESS;
}

// wake up processes sleeping on certain channel
error_code process::process_wakeup(size_t channel)
{
	spinlock_acquire(&proc_list.lock);
	auto ret = process_wakeup_nolock(channel);
	spinlock_release(&proc_list.lock);
	return ret;
}

error_code process::process_wakeup_nolock(size_t channel)
{
	list_head* iter = nullptr;
	list_for(iter, &proc_list.active_head)
	{
		auto iter_proc = list_entry(iter, process::process_dispatcher, link);
		if (iter_proc->state == PROC_STATE_SLEEPING && iter_proc->sleep_data.channel == channel)
		{
			iter_proc->state = PROC_STATE_RUNNABLE;
		}
	}
	return ERROR_SUCCESS;
}

error_code process::process_heap_change_size(IN process_dispatcher* proc, IN OUT uintptr_t* heap_ptr)
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

	spinlock_acquire(&mm->lock);

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

	spinlock_release(&mm->lock);

	return ERROR_SUCCESS;
}

