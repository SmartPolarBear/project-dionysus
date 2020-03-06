#include "sys/error.h"
#include "sys/proc.h"

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
	return proc_list.proc_count;
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

	uintptr_t stk_top = proc->kstack;
	stk_top -= sizeof(trap_frame);

	// set trap frame
	proc->trapframe = reinterpret_cast<decltype(proc->trapframe)>(stk_top);

	stk_top -= sizeof(uintptr_t);
	//set trap return point
	*((uintptr_t *)stk_top) = (uintptr_t)trap_ret;

	stk_top -= sizeof(process::process_context);
	proc->context = reinterpret_cast<decltype(proc->context)>(stk_top);
	memset(proc->context, 0, sizeof(process::process_context));
	proc->context->rip = (uintptr_t)scheduler_ret;

	proc->trapframe->rflags |= EFLAG_IF;

	if (flags & PROC_SYS_SERVER)
	{
		proc->trapframe->rflags |= EFLAG_IOPL_MASK;
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