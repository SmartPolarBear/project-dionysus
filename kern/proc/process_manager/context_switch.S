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