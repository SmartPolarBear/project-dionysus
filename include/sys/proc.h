#pragma once

#include "arch/amd64/regs.h"

#include "sys/types.h"
#include "sys/vmm.h"

namespace process
{
enum process_state
{
    PROC_STATE_UNUSED,
    PROC_STATE_EMBRYO,
    PROC_STATE_SLEEPING,
    PROC_STATE_RUNNABLE,
    PROC_STATE_RUNNING,
    PROC_STATE_ZOMBIE
};

struct process_context
{
    uint64_t rip;
    uint64_t rsp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

using pid = int64_t;

constexpr size_t PROC_NAME_LEN = 32;

// it should be enough
constexpr size_t PROC_MAX_COUNT = INT32_MAX;

constexpr pid PID_MAX = INT64_MAX;

// Per-process state
struct process_dispatcher
{
    static constexpr size_t KERNSTACK_PAGES = 8;
    static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PMM_PAGE_SIZE;

    process_state state;
    pid pid;
    size_t run_times;
    uintptr_t kstack;
    volatile bool need_rescheduled;

    process_dispatcher *parent;
    vmm::mm_struct *mm;
    trap_frame *trapframe;
    process_context context;
    uintptr_t cr3val;

    size_t flags;
    char name[PROC_NAME_LEN + 1];

    list_head proc_link;

    process_dispatcher(process::pid pid, const char *name);
    ~process_dispatcher();

    error_code init_kernel_stack();
    error_code copy_mm_from(vmm::mm_struct *src, size_t flags);
    error_code wakeup();
    error_code setup_context(size_t rsp, trap_frame *tf);

    void run();
};

extern process_dispatcher *initial;
extern __thread process_dispatcher *idle;
extern __thread process_dispatcher *current;

} // namespace process
