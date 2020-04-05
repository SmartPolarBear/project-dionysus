.code64

.global load_gdt
load_gdt:
    lgdt (%rdi)
    mov $0x40, %ax
    ltr %ax

    mov $0x08, %ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    #mov %ax,%ss

    popq %rdi
    movq $0x10,%rax
    pushq %rax
    pushq %rdi
    lretq