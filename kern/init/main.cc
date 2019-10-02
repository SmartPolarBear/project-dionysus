/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-02 21:00:55
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

#define VIDEO_START 0xb8000
#define VGA_LIGHT_GRAY 7

extern "C" void *mboot_addr; //boot.S

struct mboot_info
{
};

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
extern char _kernel_virtual_end[]; //kernel.ld
extern char _kernel_virtual_start[]; //kernel.ld

extern "C" [[noreturn]] void kmain() {
    console::printf("start=0x%x\n", _kernel_virtual_start);
    console::printf("s=0x%x\ne=0x%x\n", _kernel_virtual_end, P2V<char>((char *)(4 * 1024 * 1024)));
    bootmm_init(_kernel_virtual_end, P2V<char>((char *)(4 * 1024 * 1024)));
    console::puts("Hello world!\n");
    for (;;)
        ;
}
