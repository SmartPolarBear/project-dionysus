#include "process.hpp"

#include "arch/amd64/atomic.h"
#include "arch/amd64/cpu.h"

#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/proc.h"
#include "sys/scheduler.h"
#include "sys/types.h"
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

[[clang::optnone]] void scheduler::scheduler_halt()
{
    bool has_proc = false;

    list_head *iter = nullptr;
    list_for(iter, &proc_list.head)
    {
        auto entry = list_entry(iter, process::process_dispatcher, link);
        if (entry->state == process::PROC_STATE_RUNNING ||
            entry->state == process::PROC_STATE_RUNNABLE ||
            entry->state == process::PROC_STATE_ZOMBIE ||
            entry->state == process::PROC_STATE_SLEEPING)
        {
            has_proc = true;
            break;
        }
    }

    if (!has_proc)
    {
        KDEBUG_FOLLOWPANIC("no process to run");
    }

    current = nullptr;
    lcr3(V2P((uintptr_t)&g_kpml4t));

    spinlock_release(&proc_list.lock);

    asm volatile(
        "movq $0, %%rbp\n"
        "movq %0, %%rsp\n"
        "pushq $0\n"
        "pushq $0\n"
        // Uncomment the following line after completing exercise 13
        "sti\n"
        "spin:\n"
        "hlt\n"
        "jmp spin\n"
        :
        : "a"(cpu->tss.rsp0 /*(vmm::tss_get_rsp(cpu->get_tss(), 0)*/));
}

[[clang::optnone]] void scheduler::scheduler_yield()
{
    spinlock_acquire(&proc_list.lock);

    list_head *start = current != nullptr
                           ? current->link.next
                           : &proc_list.head;
    list_head *iter = start;
    if (iter->next != iter)
    {
        do
        {
            auto proc = list_entry(iter, process::process_dispatcher, link);

            if (proc->state == process::PROC_STATE_RUNNABLE)
            {

                process::process_run(proc);
            }

            iter = iter->next;
        } while (iter != start);
    }

    if (current != nullptr && current->state == process::PROC_STATE_RUNNING)
    {
        process::process_run(current);
    }

    // it never returns
    scheduler_halt();
}
