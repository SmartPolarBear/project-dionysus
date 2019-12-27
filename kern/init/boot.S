#include "boot/multiboot2.h"


// !!This file, more than being used by the grub2 bootloader, is also used for booting APs
// entry32mp and entry64mp is highly related to the AP booting


.code32
.global mboot_header
.global mboot_entry

.set header_len, mboot_header_end-mboot_header
.set checksum, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + header_len)
.set req_tag_len, req_tag_end-req_tag_start
.set entry_tag_len, entry_tag_end-entry_tag_start
.set addr_tag_len, addr_tag_end-addr_tag_start

.align 8
mboot_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long header_len
    .long checksum

    //multiboot tags
.align 8
entry_tag_start:
    .short MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS
    .short MULTIBOOT_HEADER_TAG_OPTIONAL
    .long entry_tag_len
    .long mboot_entry_addr
entry_tag_end:

.align 8
addr_tag_start:
    .short MULTIBOOT_HEADER_TAG_ADDRESS
    .short MULTIBOOT_HEADER_TAG_OPTIONAL
    .long addr_tag_len
    .long mboot_load_addr
    .long mboot_load_addr
    .long mboot_load_end
    .long mboot_bss_end
addr_tag_end:

.align 8
req_tag_start:
    .short MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST
    .short MULTIBOOT_HEADER_TAG_OPTIONAL
    .long req_tag_len
    .long MULTIBOOT_TAG_TYPE_MMAP
    .long MULTIBOOT_TAG_TYPE_ACPI_NEW
    .long MULTIBOOT_TAG_TYPE_ACPI_OLD
req_tag_end:

.align 8
    .short MULTIBOOT_HEADER_TAG_END
    .short 8    //no flags set
    .long 8     //self-containing size
mboot_header_end:


mboot_entry:

// temporary stack for mbi
    movl $0x10000,%esp

// push mbi address
    pushl %ebx
    pushl %eax

// zero 4 pages for our bootstrap page tables
    xor %eax, %eax
    mov $0x1000, %edi
    mov $0x5000, %ecx
    rep stosb

// P4ML[0] -> 0x2000 (PDPT-A)
    mov $(0x2000 | 3), %eax
    mov %eax, 0x1000

// P4ML[511] -> 0x3000 (PDPT-B)
    mov $(0x3000 | 3), %eax
    mov %eax, 0x1FF8

// PDPT-A[0] -> 0x4000 (PD)
    mov $(0x4000 | 3), %eax
    mov %eax, 0x2000

// PDPT-B[510] -> 0x4000 (PD)
    mov $(0x4000 | 3), %eax
    mov %eax, 0x3FF0

// PD[0..511] -> 0..1022MB
    mov $0x83, %eax
    mov $0x4000, %ebx
    mov $512, %ecx
ptbl_loop:
    mov %eax, (%ebx)
    add $0x200000, %eax
    add $0x8, %ebx
    dec %ecx
    jnz ptbl_loop

// Clear ebx for initial processor boot.
// When secondary processors boot, they'll call through
// entry32mp (from entryother), but with a nonzero ebx.
// We'll reuse these bootstrap pagetables and GDT.

    xor %ebx, %ebx

// !!the code below may ALSO be excuted for booting APs
.global entry32mp
entry32mp:

// CR3 -> 0x1000 (P4ML)
    mov $0x1000, %eax
    mov %eax, %cr3

    lgdt (gdtr64 - mboot_header + mboot_load_addr)

// Enable PAE - CR4.PAE=1
    mov %cr4, %eax
    bts $5, %eax
    mov %eax, %cr4

// enable long mode - EFER.LME=1
    mov $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

// if %ebx isn't zero, it's booting APs
// therefore we don't pop multiboot structures
    cmpl $0x0, %ebx
    jne skip_mboot_pop

// pop mbi & magic number
// using ecx and edx instead of eax and ebx to avoid conflict with AP booting
    popl %ecx
    popl %edx

skip_mboot_pop:

// enable paging
    mov %cr0, %eax
    bts $31, %eax
    mov %eax, %cr0


// shift to 64bit segment
    ljmp $8,$(entry64low - mboot_header + mboot_load_addr)

.align 16
gdtr64:
    .word gdt64_end - gdt64_begin - 1;
    .quad gdt64_begin - mboot_header + mboot_load_addr

.align 16
gdt64_begin:
    .long 0x00000000 # 0: null desc
    .long 0x00000000
    .long 0x00000000 # 1: Code, R/X, Nonconforming
    .long 0x00209800
    .long 0x00000000 # 2: Data, R/W, Expand Down
    .long 0x00009000
gdt64_end:

.align 16
.code64
entry64low:

// check whether it's booting APs, skip saving mbi if it is.
    cmp $0x0, %rbx
    jne skip_save_mbi

// else save the mbi & the magic number
    movq %rdx, mbi_structptr
    movq %rcx, mbi_magicnum

//  clear the registers for mbi saving
    xorq %rcx,%rcx
    xorq %rdx,%rdx

skip_save_mbi:

    movq $entry64high, %rax
    jmp *%rax

.global _start
_start:
entry64high:

// ensure data segment registers are sane
    xor %rax, %rax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

// check to see if it's booting an AP
    test %rbx, %rbx
    jnz entry64mp

// else continue the kernel initialization
// setup initial stack
    mov $0xFFFFFFFF80010000, %rax
    mov %rax, %rsp

// call global constructors
    call call_ctors

// enter main()
    call kmain

// call global destructors
    call call_dtors

// kernel shouldn't return here
spin:
  jmp spin

entry64mp:
// obtain kstack from data block before entryother
    mov $0x7000, %rax
    mov -16(%rax), %rsp
    jmp ap_enter

.global wrmsr
wrmsr:
    mov %rdi, %rcx     // arg0 -> msrnum
    mov %rsi, %rax     // val.low -> eax
    shr $32, %rsi
    mov %rsi, %rdx     // val.high -> edx
    wrmsr
    retq

