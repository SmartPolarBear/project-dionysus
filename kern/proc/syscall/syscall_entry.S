#include "syscall.h"

.code64

// we don't use C++ here because it unexpectedly modifies some registers.
// asm make a function rather 'clean'

.global syscall_x64_entry
syscall_x64_entry:

    swapgs

    movq %rsp, %gs:(KERNEL_GS_USTACK)
    movq %gs:(KERNEL_GS_KSTACK), %rsp

    pushq %gs:(KERNEL_GS_USTACK)
    movl $0x123caffe, %gs:(KERNEL_GS_USTACK)

    swapgs

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

    movq %rsp, %rdi  // first parameter: pointer to regs
    call syscall_body

    // we discard rax because it is used to store return value
    addq $8, %rsp

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

    popq %rsp

    sysretq