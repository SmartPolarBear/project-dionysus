#pragma once

#include "arch/amd64/regs.h"

#include "system/process.h"

#include "drivers/lock/spinlock.h"
#include "drivers/apic/traps.h"

#include "data/Queue.hpp"

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

//extern __thread process::process_dispatcher *current;
extern CLSItem<process::process_dispatcher*, CLS_PROC_STRUCT_PTR> cur_proc;


struct process_list_struct
{
	spinlock lock;
	size_t proc_count;
	list_head active_head;
	libkernel::Queue<process::process_dispatcher*> zombie_queue;
};

extern process_list_struct proc_list;


[[maybe_unused]] extern "C" [[noreturn]] void run_process(trap::trap_frame* tf, uintptr_t kstack);

extern "C" [[noreturn]] void user_proc_entry();

extern "C" void context_switch(context** oldcontext, context* newcontext);

// process.cc
process::process_dispatcher* find_process(process_id pid);


