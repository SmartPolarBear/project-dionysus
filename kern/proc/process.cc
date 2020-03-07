#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/proc.h"
#include "sys/vmm.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "lib/libkern/data/list.h"

using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

__thread process::process_dispatcher *current;

// scheduler.cc
void scheduler_ret();

struct
{
	spinlock lock;
	list_head head;
	size_t proc_count;
} proc_list;

static inline process::pid alloc_pid(void)
{
	return proc_list.proc_count++;
}

static inline error_code setup_mm(process::process_dispatcher *proc)
{
	proc->mm = vmm::mm_create();
	if (proc->mm == nullptr)
	{
		return ERROR_MEMORY_ALLOC;
	}

	vmm::pde_ptr_t pgdir = reinterpret_cast<vmm::pde_ptr_t>(new BLOCK<PMM_PAGE_SIZE>);
	if (proc->mm == nullptr)
	{
		vmm::mm_destroy(proc->mm);
		return ERROR_MEMORY_ALLOC;
	}

	memmove(pgdir, g_kpml4t, PMM_PAGE_SIZE);

	proc->mm->pgdir = pgdir;

	return ERROR_SUCCESS;
}

void process::process_init(void)
{
	proc_list.proc_count = 0;
	spinlock_initlock(&proc_list.lock, "proc");
	list_init(&proc_list.head);
}

error_code process::create_process(IN const char *name,
								   IN size_t flags,
								   IN bool inherit_parent,
								   OUT process_dispatcher *proc)
{

	if (proc_list.proc_count >= process::PROC_MAX_COUNT)
	{
		return ERROR_TOO_MANY_PROC;
	}

	proc = new process_dispatcher(name, alloc_pid(), current->id, flags);

	//setup kernel stack
	proc->kstack = (uintptr_t) new BLOCK<process::process_dispatcher::KERNSTACK_SIZE>;

	if (!proc->kstack)
	{
		delete proc;
		return ERROR_MEMORY_ALLOC;
	}

	error_code ret = ERROR_SUCCESS;
	if ((ret = setup_mm(proc)) != ERROR_SUCCESS)
	{
		delete proc;
		return ret;
	}


	proc->trapframe.cs = (SEG_UCODE << 3) | DPL_USER;
	proc->trapframe.ss = (SEG_UDATA << 3) | DPL_USER;
	proc->trapframe.rsp = USER_STACK_TOP;

	proc->trapframe.rflags |= EFLAG_IF;

	if (flags & PROC_SYS_SERVER)
	{
		proc->trapframe.rflags |= EFLAG_IOPL_MASK;
	}

	if (inherit_parent)
	{
		// TODO: copy data from parent process
	}

	spinlock_acquire(&proc_list.lock);

	list_add(&proc->link, &proc_list.head);

	spinlock_release(&proc_list.lock);

	return ERROR_SUCCESS;
}

error_code process::process_load_binary(IN process_dispatcher *porc,
										IN uint8_t *bin,
										IN size_t binsize,
										IN binary_types type)
{
}

error_code process::process_run(IN process_dispatcher *porc)
{
}