.code64

.global swap_gs
swap_gs:
    swapgs
    ret

.global safe_swap_gs
safe_swap_gs:
    // TODO: check if already in kernel
    // cmpq $0x08, 0x8(%rsp)
    // je end_safe_swap_gs
    call swap_gs
end_safe_swap_gs:
    ret

.global load_gdt_and_tr
load_gdt_and_tr:
    lgdt (%rdi)
    movw %si, %ax
    ltr %ax

    mov $0x10, %ax
    mov %ax,%ss

    popq %rdi
    movq $0x08,%rax
    pushq %rax
    pushq %rdi
    lretq

.global cli
cli:
    cli
    ret

.global sti
sti:
    sti
    ret

.global hlt
hlt:
    hlt
    ret