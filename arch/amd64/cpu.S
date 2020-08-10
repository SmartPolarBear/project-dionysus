.code64

.global swap_gs
swap_gs:
    swapgs
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

.global enable_avx
enable_avx:
    push %rax
    push %rcx
    push %rdx

    xor %rcx,%rcx
    xgetbv
    or $0x7, %eax
    xsetbv

    pop %rdx
    pop %rcx
    pop %rax
    ret
