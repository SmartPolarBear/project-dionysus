#include "proc.h"
#include "arch/amd64/regs.h"

#include "sys/error.h"
#include "sys/pmm.h"
#include "sys/proc.h"
#include "sys/types.h"

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"
#include "lib/libkern/data/list.h"

process::process_dispatcher::process_dispatcher(process::pid _pid, const char *name)
{
    this->state = PROC_STATE_EMBRYO;

    this->pid = _pid;
    this->cr3val = rcr3();

    memset(this->name, 0, sizeof(this->name));
    strncpy(this->name, name, sysstd::min(process::PROC_NAME_LEN, (size_t)strlen(name)));

    this->run_times = 0;
    this->kstack = 0;
    this->need_rescheduled = false;
    this->parent = nullptr;
    this->mm = nullptr;
    this->trapframe = nullptr;
    this->flags = 0;
    memset(&(this->context), 0, sizeof(this->context));
}

process::process_dispatcher::~process_dispatcher()
{
    if (this->kstack != 0)
    {
        auto pages = pmm::va_to_page(this->kstack);
        pmm::free_pages(pages, KERNSTACK_PAGES);
    }
}

error_code process::process_dispatcher::wakeup()
{
    if (this->state == PROC_STATE_ZOMBIE || this->state == PROC_STATE_RUNNABLE)
    {
        this->state = PROC_STATE_RUNNABLE;
    }
}

// kernel code will be automatically freed in dtor
error_code process::process_dispatcher::init_kernel_stack(void)
{
    auto pages_kstack = pmm::alloc_pages(KERNSTACK_PAGES);
    if (pages_kstack != nullptr)
    {
        this->kstack = (uintptr_t)pmm::page_to_va(pages_kstack);
        return ERROR_SUCCESS;
    }
    return -ERROR_MEMORY_ALLOC;
}

error_code process::process_dispatcher::copy_mm_from(vmm::mm_struct *src, size_t flags)
{
    return ERROR_SUCCESS;
}

error_code process::process_dispatcher::setup_context(size_t rsp, trap_frame *tf)
{
    uintptr_t kstacktop = this->kstack + KERNSTACK_SIZE;
    this->trapframe = ( trap_frame *)kstacktop - 1;
    *(this->trapframe) = *tf;
    this->trapframe->rax = 0;
    this->trapframe->rsp = (rsp != 0) ? rsp : kstacktop;
    this->trapframe->rflags |= EFLAG_IF;

    this->context.rip = (uintptr_t)childproc_ret;
    this->context.rsp = (uintptr_t)(this->trapframe);

    return ERROR_SUCCESS;
}