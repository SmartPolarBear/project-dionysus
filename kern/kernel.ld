/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-25 23:45:39
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-01 15:24:05
 * @ Description:
 */


ENTRY(_start)

SECTIONS
{
    . = 4M;
    _kernel_physical_start = .;

    .boottext :
    {
        build/kern/init/boot.o (.multiboot)
        build/kern/init/boot.o (.text)
    }

    .bootrodata :
    {
        build/kern/init/boot.o (.rodata)
    }

    .bootdata :
    {
        build/kern/init/boot.o (.data)
    }

    .bootbss :
    {
        build/kern/init/boot.o (.bss)
        build/kern/init/boot.o (COMMON)
    }

    . = ALIGN(0x1000);
    _boot_end = .;

    . += 0xFFFFFFFF80000000;
    _kernel_virtual_start = .;
    .text : AT(_boot_end)
    {
      *(.multiboot)
      *(.text)
    }
  
    . = ALIGN(0x1000);
  
    .rodata : AT ( (LOADADDR (.text) + SIZEOF (.text) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
      *(.rodata)
    }
  
    . = ALIGN(0x1000);
  
    .data : AT ( (LOADADDR (.rodata) + SIZEOF (.rodata) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
      start_ctors = .;
      *(.ctor*)
      end_ctors = .;
      start_dtors = .;
      *(.dtor*)
      end_dtors = .;
      *(.data)
    }
  
    . = ALIGN(0x1000);
  
    .bss : AT ( (LOADADDR (.data) + SIZEOF (.data) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
    {
      *(COMMON)
      *(.bss)
    }
  
    _kernel_virtual_end = .;
  
    _kernel_physical_end = (LOADADDR (.bss) + SIZEOF (.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}  