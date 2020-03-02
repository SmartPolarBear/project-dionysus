#include "proc.h"

#include "arch/amd64/regs.h"
#include "arch/amd64/sync.h"

#include "sys/error.h"
#include "sys/pmm.h"
#include "sys/proc.h"
#include "sys/types.h"

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"
#include "lib/libkern/data/list.h"

using libk::list_add;
using libk::list_add_tail;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_holding;
using lock::spinlock_initlock;
using lock::spinlock_release;

using process::process_dispatcher;

// process.cc
extern process_list plist;

void schedule()
{
    bool intr_store = false;
    local_intrrupt_save(intr_store);
    {
        spinlock_acquire(&plist.lock);

        process::current->need_rescheduled = false;

        list_head *last = (process::current == process::idle) ? &plist.proc_head : &(process::current->proc_link);
        list_head *iter = nullptr;
        process_dispatcher *proc = nullptr;

        do
        {
            if ((iter = last->next) != &plist.proc_head)
            {
                auto p = list_entry(iter, process_dispatcher, proc_link);
                if (p->state == process::PROC_STATE_RUNNABLE)
                {
                    proc = p;
                    break;
                }
            }
        } while (iter != last);

        if(proc==nullptr||proc->state!=process::PROC_STATE_RUNNABLE)
        {
            proc = process::idle;
        }
        proc->run_times += 1;

        if(proc!=process::current)
        {
            proc->run();
        }

        spinlock_release(&plist.lock);
    }
    local_intrrupt_restore(intr_store);
}
