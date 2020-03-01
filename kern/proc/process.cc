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

// trapentry_asm.S
extern "C" void childproc_ret_asm(trap_frame *tf);

// entrypoint for proc's rip
void childproc_ret()
{
    childproc_ret_asm(current->trapframe);
}

process_dispatcher *initial = nullptr;
__thread process_dispatcher *idle = nullptr;
__thread process_dispatcher *current = nullptr;

process_list plist{};

// precondition: the plist's lock is held
// ret: the variable to hold the new pid
static inline error_code alloc_pid(OUT process::pid &ret)
{
    if (!spinlock_holding(&plist.lock))
    {
        return -ERROR_LOCK_STATUS;
    }

    ret = plist.proc_count++;
    if (ret == process::PID_MAX)
    {
        return -ERROR_TOO_MANY_PROC;
    }

    return ERROR_SUCCESS;
}

static inline error_code proc_alloc_and_insert(const char *name, OUT process::process_dispatcher *ret)
{
    error_code err = ERROR_SUCCESS;

    spinlock_acquire(&plist.lock);

    process::pid pid = 0;
    if ((err = alloc_pid(pid)) != ERROR_SUCCESS)
    {
        spinlock_release(&plist.lock);
        return err;
    }

    auto proc = new process::process_dispatcher(pid, name);

    list_add(&proc->proc_link, &plist.proc_head);

    spinlock_release(&plist.lock);

    proc->parent = current;

    ret = proc;

    return ERROR_SUCCESS;
}

error_code create_process(char *name, size_t flags, uintptr_t stack, trap_frame *tf,
                          OUT process::process_dispatcher *proc)
{
    error_code err = ERROR_SUCCESS;

    if ((err = proc_alloc_and_insert(name, proc)) != ERROR_SUCCESS)
    {
        return err;
    }

    if ((err = proc->init_kernel_stack()) != ERROR_SUCCESS)
    {
        delete proc;
        return err;
    }

    if ((err = proc->copy_mm_from(current->mm, flags)) != ERROR_SUCCESS)
    {
        delete proc;
        return err;
    }

    if ((err = proc->setup_context(stack, tf)) != ERROR_SUCCESS)
    {
        delete proc;
        return err;
    }

    return proc->wakeup();
}
