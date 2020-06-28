.code64

.global swap_gs
swap_gs:
    swapgs
    ret

.global safe_swap_gs
safe_swap_gs:
    // TODO: check if already in kernel
    //cmpq $0x08, %cs
    //je end_safe_swap_gs
    call swap_gs
end_safe_swap_gs:
    ret

.global load_gdt
load_gdt:
    lgdt (%rdi)

    mov $0x10, %ax
    mov %ax,%ss
    mov %ax,%es
    mov %ax,%ds

    popq %rdi
    movq $0x08,%rax
    pushq %rax
    pushq %rdi

    lretq

.global load_tr
load_tr:
    movw %di, %ax
    ltr %ax
    ret

.global load_idt
load_idt:
    lidt (%rdi)
    ret

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