#pragma once

#include "arch/amd64/cpu/regs.h"

#include "system/process.h"

#include "drivers/lock/spinlock.h"
#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

#include "kbl/queue.hpp"

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;

//extern __thread task::process_dispatcher *current;
extern CLSItem<task::process_dispatcher*, CLS_PROC_STRUCT_PTR> cur_proc;

struct process_list_struct
{
	spinlock_struct lock;
	lock::spinlock_lockable lockable{ lock };

	size_t proc_count;
	list_head active_head;
	task::task_dispatcher::head_type head;
	libkernel::queue<task::process_dispatcher*> zombie_queue;
};

extern process_list_struct proc_list;

extern "C" [[noreturn]] void user_proc_entry();

extern "C" void context_switch(context** oldcontext, context* newcontext);

// task.cc
task::process_dispatcher* find_process(process_id pid);


