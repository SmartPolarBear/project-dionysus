.code64

// extern "C"  void context_switch(struct context **old, struct context *new)
.global context_switch
context_switch:
      // Save old callee-save registers
      push %rbp
      push %rbx
      push %r11
      push %r12
      push %r13
      push %r14
      push %r15

      // Switch stacks
      mov %rsp, (%rdi)
      mov %rsi, %rsp

      // Load new callee-save registers
      pop %r15
      pop %r14
      pop %r13
      pop %r12
      pop %r11
      pop %rbx
      pop %rbp

      ret // run code from address in %rip

// assume that the stack is firstly kstack pointer and then a trapframe
.global user_proc_entry
user_proc_entry:
    popq %rsi   // pop kstack address, we currently do nothing for it.

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
.global thread_entry
thread_entry:
    popq %rsi   // pop kstack address, we currently do nothing for it.

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