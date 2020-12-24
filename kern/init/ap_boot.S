#include "system/mmu_asm.h"

.code16
.globl start
start:
    cli

    xorw    %ax,%ax
    movw    %ax,%ds
    movw    %ax,%es
    movw    %ax,%ss

    lgdt    gdtdesc16
    movl    %cr0, %eax
    orl     $0x00000001, %eax //0x00000001 enables the protected mode
    movl    %eax, %cr0

    ljmpl    $(1<<3), $(start32) //1: kernel code section

.code32
start32:
    movw    $(2<<3), %ax //2: kernel kbl section
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    $0, %ax
    movw    %ax, %fs
    movw    %ax, %gs

// flags for AP booting in boot.S
    mov     $0x3f3f3f3f, %ebx

    
// Switch to the stack
    movl    (start-4), %esp

// Call the assigned AP booting procedure
    call	 *(start-8)

    movw    $0x8a00, %ax
    movw    %ax, %dx
    outw    %ax, %dx
    movw    $0x8ae0, %ax
    outw    %ax, %dx

spin:
    jmp     spin

.p2align 2
gdt16:
    SEG_NULLASM
    SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
    SEG_ASM(STA_W, 0, 0xffffffff)

gdtdesc16:
    .word   (gdtdesc16 - gdt16 - 1)
    .long   gdt16


