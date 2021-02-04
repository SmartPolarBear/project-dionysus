#include "syscall.h"
.code64


// extern "C"  void context_switch(struct context **old, struct context *new)
.global context_switch
context_switch:
      // Save old callee-save registers
      pushq %rbp
      pushq %rbx
      pushq %r11
      pushq %r12
      pushq %r13
      pushq %r14
      pushq %r15

      // Switch stacks
      movq %rsp, (%rdi)
      movq %rsi, %rsp

      // Load new callee-save registers
      popq %r15
      popq %r14
      popq %r13
      popq %r12
      popq %r11
      popq %rbx
      popq %rbp

      retq // run code from address in %rip

.global user_entry
user_entry:
    popq %rsi   // pop kstack_ address, we currently do nothing for it.

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

.global thread_trampoline_s
thread_trampoline_s:
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

   // discard trapnum and errorcode
   addq $16, %rsp

   iretq


.global thread_entry
thread_entry:
    movq %rdi, %rax         // mov fn to rax
    movq %rsi, %rdi         // prepare arg

    movq %rdx, %r12         // put exit callback in preserved register

    callq *%rax             // call fn

    movq %rax, %rdi         // save the return value of fn(arg)
    call *%r12              // call exit callback to terminate current thread