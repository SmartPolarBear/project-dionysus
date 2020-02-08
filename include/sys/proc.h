
#if !defined(__INCLUDE_SYS_PROC_H)
#define __INCLUDE_SYS_PROC_H

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
    size_t r15;
    size_t r14;
    size_t r13;
    size_t r12;
    size_t r11;
    size_t rbx;
    size_t rbp;
    size_t rip;
};

// Per-process state
struct process
{
    char name[16];   // Process name (debugging)
    uint32_t killed; // If non-zero, have been killed
    size_t pid;      // Process ID
    uintptr_t size;  // Size of process memory (bytes)

    vmm::pde_t *pgdir;    // Page table
    char *kernel_stack;  // Bottom of kernel stack for this process
    void *sleep_channel; // If non-zero, sleeping on channel

    process_state state;      // Process state
    trap_info *trapinfo;      // Trap frame for current syscall
    process_context *context; // swtch() here to run process
    process *parent;          // Parent process

    // struct file *ofile[NOFILE]; // Open files
    // struct inode *cwd;          // Current directory
};

} // namespace process

#endif // __INCLUDE_SYS_PROC_H
