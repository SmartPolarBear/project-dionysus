.code64

#include "system/syscall.h"

#define SYSCALL_INTERFACE(name) \
  .globl name; \
  name: \
    mov $SYS_ ## name, %rax; \
    syscall;

SYSCALL_INTERFACE(hello)