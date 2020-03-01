.globl trap_entry
trap_entry:
  # Build a trap_info structure.
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rbp
  push %rdx
  push %rcx
  push %rbx
  push %rax

  mov  %rsp, %rdi  # frame in arg1
  call trap_body

  # Return falls through to trap_ret
  
.globl trap_ret
trap_ret:
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rbp
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15

  # discard trapnum and errorcode
  add $16, %rsp
  iretq


.globl childproc_ret_asm
childproc_ret_asm:
    # set stack to this new process's trapframe
    movq %rdi, %rsp
    jmp trap_ret