/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-03 12:34:02
 * @ Description: the entry point for kernel in C++
 */

#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"
#include "drivers/console/cga.h"
#include "drivers/console/console.h"
#include "lib/libc/string.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/param.h"

extern "C" void *mboot_addr; //boot.S
extern char _kernel_virtual_start[]; //kernel.ld
extern char _kernel_virtual_end[];   //kernel.ld

//C++ ctors
extern "C"
{
    using constructor_type = void (*)();
    extern constructor_type start_ctors;
    extern constructor_type end_ctors;

    void call_ctors(void)
    {
        for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
        {
            (*ctor)();
        }
    }
}

extern "C" [[noreturn]] void kmain() {
    bootmm_init(_kernel_virtual_end, _kernel_virtual_end + 0x100000);
    console::puts("Hello world!\n");
    for (;;)
        ;
}
