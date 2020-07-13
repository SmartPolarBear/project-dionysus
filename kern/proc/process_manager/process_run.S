#include "syscall.h"

.code64


// void do_run_process(trap::trap_frame tf, uintptr_t kstack)

.global do_run_process
do_run_process:
    call safe_swap_gs

    movq %rsi, %gs:(KERNEL_GS_KSTACK)

    call safe_swap_gs

    movq %rdi, %rsp

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rbp
    popq %rsi
    popq %rdi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    addq $16, %rsp

    // rip into rcx
    popq %rcx

    addq $8, %rsp

    // rflags into r11
    popq %r11

    // rsp into rsp
    popq %rsp

    // switch to user model
    sysretq


// assume that the stack is firstly kstack pointer and then a trapframe
.global syscall_ret
syscall_ret:
    popq %rsi   // pop kstack address

    call safe_swap_gs

    movq %rsi, %gs:(KERNEL_GS_KSTACK)

    call safe_swap_gs

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rbp
    popq %rsi
    popq %rdi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    addq $16, %rsp

    // rip into rcx
    popq %rcx

    addq $8, %rsp

    // rflags into r11
    popq %r11

    // rsp into rsp
    popq %rsp

    // switch to user model
    sysretq
