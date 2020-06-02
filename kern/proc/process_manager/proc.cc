/*
 * Last Modified: Sun May 17 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#include "process.hpp"
#include "elf.hpp"
#include "syscall.h"

#include "arch/amd64/cpu.h"
#include "arch/amd64/msr.h"

#include "system/error.h"
#include "system/kmalloc.h"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/proc.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "libraries/libkern/data/list.h"
#include "libraries/libkernel/console/builtin_console.hpp"

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

process_list_struct proc_list;

// precondition: the lock must be held
static inline process::pid alloc_pid(void)
{
    return proc_list.proc_count++;
}

static inline error_code setup_mm(process::process_dispatcher *proc)
{
    proc->mm = vmm::mm_create();
    if (proc->mm == nullptr)
    {
        return -ERROR_MEMORY_ALLOC;
    }

    vmm::pde_ptr_t pgdir = vmm::pgdir_entry_alloc();

    if (pgdir == nullptr)
    {
        vmm::mm_destroy(proc->mm);
        return -ERROR_MEMORY_ALLOC;
    }

    memmove(pgdir, g_kpml4t, PGTABLE_SIZE);

    proc->mm->pgdir = pgdir;

    return ERROR_SUCCESS;
}

void process::process_init(void)
{
    proc_list.proc_count = 0;
    spinlock_initlock(&proc_list.lock, "proc");
    list_init(&proc_list.run_head);
}

error_code process::create_process(IN const char *name,
                                   IN size_t flags,
                                   IN bool inherit_parent,
                                   OUT process_dispatcher **retproc)
{
    spinlock_acquire(&proc_list.lock);

    if (proc_list.proc_count >= process::PROC_MAX_COUNT)
    {
        return -ERROR_TOO_MANY_PROC;
    }

    auto proc = new process_dispatcher(name, alloc_pid(), current == nullptr ? 0 : current->id, flags);

    //setup kernel stack
    proc->kstack = (uintptr_t) new BLOCK<process::process_dispatcher::KERNSTACK_SIZE>;

    if (!proc->kstack)
    {
        delete proc;
        return -ERROR_MEMORY_ALLOC;
    }

    error_code ret = ERROR_SUCCESS;
    if ((ret = setup_mm(proc)) != ERROR_SUCCESS)
    {
        delete proc;
        return ret;
    }

    // proc->trapframe.cs = (SEG_UCODE << 3) | DPL_USER;
    // proc->trapframe.ss = (SEG_UDATA << 3) | DPL_USER;
    //TODO: load real user segment selectors
    proc->trapframe.cs = 8;
    proc->trapframe.ss = 8 + 8;
    proc->trapframe.rsp = USER_STACK_TOP;

    proc->trapframe.rflags |= trap::EFLAG_IF;

    if (flags & PROC_SYS_SERVER)
    {
        proc->trapframe.rflags |= trap::EFLAG_IOPL_MASK;
    }

    if (inherit_parent)
    {
        // TODO: copy data from parent process
    }

    list_add(&proc->link, &proc_list.run_head);

    spinlock_release(&proc_list.lock);

    *retproc = proc;
    return ERROR_SUCCESS;
}

error_code process::process_load_binary(IN process_dispatcher *proc,
                                        IN uint8_t *bin,
                                        [[maybe_unused]] IN size_t binsize OPTIONAL,
                                        IN binary_types type)
{
    error_code ret = ERROR_SUCCESS;

    uintptr_t entry_addr = 0;
    if (type == BINARY_ELF)
    {

        ret = elf_load_binary(proc, bin, &entry_addr);
    } else
    {
        ret = -ERROR_INVALID_ADDR;
    }

    if (ret == ERROR_SUCCESS)
    {
        proc->trapframe.rip = entry_addr;

        // allocate an stack
        for (size_t i = 0; i < process::process_dispatcher::KERNSTACK_PAGES; i++)
        {
            uintptr_t va = USER_TOP - process::process_dispatcher::KERNSTACK_SIZE + i * PAGE_SIZE;
            page_info *page_ret = nullptr;
            auto ret = pmm::pgdir_alloc_page(proc->mm->pgdir, true, va, PG_W | PG_U | PG_PS | PG_P, &page_ret);
            if (ret != ERROR_SUCCESS)
            {
                return -ERROR_MEMORY_ALLOC;
            }
        }

        proc->state = PROC_STATE_RUNNABLE;
    }

    return ret;
}

error_code process::process_run(IN process_dispatcher *proc)
{

    if (current != nullptr && current->state == PROC_STATE_RUNNING)
    {
        current->state = PROC_STATE_RUNNABLE;
    }

    trap::pushcli();

    current = proc;
    current->state = PROC_STATE_RUNNING;
    current->runs++;

    KDEBUG_ASSERT(current != nullptr && current->mm != nullptr);

    lcr3(V2P((uintptr_t) current->mm->pgdir));

    spinlock_release(&proc_list.lock);

    // vmm::tss_set_rsp(cpu->get_tss(), 0, current->kstack + process_dispatcher::KERNSTACK_SIZE);
#ifndef USE_NEW_CPU_INTERFACE
    cpu->tss.rsp0 = current->kstack + process_dispatcher::KERNSTACK_SIZE;
#else
    cpu()->tss.rsp0 = current->kstack + process_dispatcher::KERNSTACK_SIZE;
#endif

    trap::popcli();

    do_run_process(&current->trapframe, current->kstack);

    KDEBUG_FOLLOWPANIC("iret failed");
}

void process::process_terminate()
{
    process_terminate(current);
}

void process::process_terminate(IN process_dispatcher *proc)
{
    // TODO: close all files

    // TODO: wakeup parent and inform it of the termination

    // TODO: recycle the memory




    // set process state and call the scheduler
    proc->state = PROC_STATE_ZOMBIE;

    scheduler::scheduler_yield();

    KDEBUG_GENERALPANIC("process_terminate: should not reach here.");
}

