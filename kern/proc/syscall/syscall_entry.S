#include "syscall.h"

.code64

.global syscall_x64_entry
syscall_x64_entry:
    call safe_swap_gs

    call syscall_body

    call safe_swap_gs
    sysretq