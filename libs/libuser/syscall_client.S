.code64

#include "system/syscall.h"

// According to System V ABI
// Parameters are in rdi, rsi, rdx, rcx, r8, r9, stack (right to left)
// Return values are in rax and rdx

#define SYSCALL_INTERFACE(name) \
  .globl name; \
  name: \
    mov $SYS_ ## name, %rax; \
    syscall;

SYSCALL_INTERFACE(hello)