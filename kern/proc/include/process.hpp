#pragma once

#include "sys/proc.h"

#include "drivers/lock/spinlock.h"

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

struct process_list_struct
{
    spinlock lock;
    list_head head;
    size_t proc_count;
};

extern process_list_struct proc_list;

extern __thread process::process_dispatcher *current;
