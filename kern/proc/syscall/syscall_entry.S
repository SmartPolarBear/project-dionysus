#include "syscall.h"

.code64

.global syscall_x64_entry
syscall_x64_entry:
    call safe_swap_gs

    movq %rsp, %gs:(KERNEL_GS_USTACK)
    movq %gs:(KERNEL_GS_KSTACK), %rsp


    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rdi
    pushq %rsi
    pushq %rbp
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    call syscall_body

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

    movq %gs:(KERNEL_GS_USTACK), %rsp

    call safe_swap_gs
    sysretq