#pragma once

#include "sys/types.h"
#include "drivers/lock/spinlock.h"
#include "lib/libkern/data/list.h"

struct process_list
{
    lock::spinlock lock;
    list_head proc_head;
    size_t proc_count;

    process_list()
    {
        libk::list_init(&this->proc_head);
        lock::spinlock_initlock(&this->lock, "process_list");
        proc_count = 0;
    }
};

// process.cc
extern process_list plist;

extern void childproc_ret();