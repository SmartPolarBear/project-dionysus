#include "sys/multiboot2.h"
#include "sys/asm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/param.h"
#include "sys/msr.h"
#include "sys/cr.h"

.set header_len, header_end-header_start
.set checksum, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + header_len)

.section .multiboot
header_start:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long header_len
    .long checksum

    //current no multiboot tags

    .short MULTIBOOT_HEADER_TAG_END
    .short 8    //no flags set
    .long 8     //self-containing size
header_end:

.code32

.section .bss
.comm pml4      , PML4_SIZE         , PML4_ALIGN
.comm lo_pdpt   , PDPT_SIZE         , PDPT_ALIGN
.comm hi_pdpt   , PDPT_SIZE         , PDPT_ALIGN
.comm lo_pdt    , PD_SIZE           , PD_ALIGN
.comm hi_pdt    , PD_SIZE           , PD_ALIGN
.comm tstack     , BOOT_STACK_SIZE   , BOOT_STACK_ALIGN

.data
// Bootstrap GDT
.p2align 2                                // force 4 byte alignment
gdt:
  .8byte BOOTGHT_FIRST_ENTRY                             // null seg
  .8byte BOOTGHT_SECOND_ENTRY

gdtdesc:
  .word   (gdtdesc - gdt - 1)             // sizeof(gdt) - 1
  .long   gdt                             // address gdt

.section .text

.global _start
.type _start, @function

_start:
    movl $tstack + BOOT_STACK_SIZE, %esp
    
    movl $lo_pdpt,%eax
    or $(PG_P|PG_W),%eax
    movl %eax,pml4+(PML4_ADDR2ENTRYID(KPHYSICAL)*PML4_ENTRY_SIZE)

    movl $hi_pdpt,%eax
    or $(PG_P|PG_W),%eax
    movl %eax,pml4+(PML4_ADDR2ENTRYID(KVIRTUAL)*PML4_ENTRY_SIZE)

    movl $lo_pdt,%eax
    or $(PG_P|PG_W),%eax
    movl %eax,lo_pdpt+(PDPT_ADDR2ENTRYID(KPHYSICAL)*PDPT_ENTRY_SIZE)

    movl $hi_pdt,%eax
    or $(PG_P|PG_W),%eax
    movl %eax,hi_pdpt+(PDPT_ADDR2ENTRYID(KVIRTUAL)*PDPT_ENTRY_SIZE)

    mov $0,%ecx

    movl $_kernel_physical_end,%esi
    shrl $21, %esi //Set 2MB bit
    addl $1,%esi


pdt_setup:
    movl $(1<<21),%eax
    mul %ecx
    or $(PG_P|PG_W|PG_2MB),%eax
    movl %eax,lo_pdt(,%ecx,PD_ENTRY_SIZE)
    movl %eax,hi_pdt(,%ecx,PD_ENTRY_SIZE)

    inc %ecx
    cmp %esi,%ecx
    jne pdt_setup  //loop until %esi==%ecx

    movl $pml4,%eax
    movl %eax,%cr3  //install page

    movl $(1<<5),%eax
    movl %eax,%cr4  //set PAE enable
    
    movl $MSR_EFER,%ecx
    rdmsr
    or $MSR_EFER_LME,%eax
    wrmsr

    movl $BOOTCR0,%eax
    movl %eax,%cr0

    lgdt gdtdesc

    ljmp $(KGDT_ENTRY*GDT_ENTRY_SIZE),$_start64

    cli
    hlt

.code64

.global _start64
.type _start64,@function
_start64:
    movw $0,%ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    call kmain //transfer control to the kernel

    //main should never return
    cli
    hlt
spin:
    jmp spin