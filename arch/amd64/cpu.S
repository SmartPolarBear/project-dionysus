.code64

.global swap_gs
swap_gs:
    swapgs
    ret

.global safe_swap_gs
safe_swap_gs:
    cmpq $0x08, 0x8(%rsp)
    je end_safe_swap_gs
    call swap_gs
end_safe_swap_gs:
    ret

.global load_gdt
load_gdt:
    lgdt (%rdi)
    mov $0x40, %ax
    ltr %ax

    mov $0x10, %ax
    ;// mov %ax,%ds
    ;// mov %ax,%es
    ;// mov %ax,%fs
    ;// mov %ax,%gs
    mov %ax,%ss

    popq %rdi
    movq $0x08,%rax
    pushq %rax
    pushq %rdi
    lretq