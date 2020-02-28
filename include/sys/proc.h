#pragma once

#include "arch/amd64/regs.h"

#include "sys/types.h"
#include "sys/vmm.h"


namespace process
{
enum process_state
{
    PS_UNUSED,
    PS_EMBRYO,
    PS_SLEEPING,
    PS_RUNNABLE,
    PS_RUNNING,
    PS_ZOMBIE
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

using pid = size_t;

constexpr size_t PROC_NAME_LEN = 32;

// it should be enough
constexpr size_t PROC_MAX_COUNT = INT32_MAX;

constexpr pid PID_MAX = INT64_MAX;

// Per-process state
struct process_ctl
{
    process_state state;
    pid pid;
    size_t run_times;
    uintptr_t kstack;
    volatile bool need_rescheduled;

    process_ctl *parent;
    vmm::mm_struct mm;
    process_context context;
    trap_frame *trapframe;
    uintptr_t cr3val;

    size_t flags;
    char name[PROC_NAME_LEN + 1];

    list_head *proc_link;
};

} // namespace process

