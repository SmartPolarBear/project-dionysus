#pragma once

#include "internals/thread.hpp"

#include "arch/amd64/cpu/regs.h"


#include "task/thread/thread.hpp"

#include "kbl/lock/spinlock.h"

#include "ktl/list.hpp"

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;

//extern __thread task::process *current;
extern cls_item<task::process*, CLS_PROC_STRUCT_PTR> cur_proc;

struct process_list_struct
{
	lock::spinlock lock{"TaskLock"};
	// FIXME: to be delete soon
};

extern process_list_struct proc_list;

// task.cc
task::process* find_process(pid_type pid);


