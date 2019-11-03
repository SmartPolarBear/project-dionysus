/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-03 20:46:20
 * @ Description: the entry point for kernel in C++
 */

#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"

#include "drivers/acpi/acpi.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new.h"

#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/vm.h"

extern char end[]; // kernel.ld

// global entry of the kernel
extern "C" [[noreturn]] void kmain() {
    // memory allocator at boot time, allocating memory within (end+0x1000,P2V(4MB))
    // the offset is intended to protect multiboot info from overwritten,
    // which is put *right* after the kernel by grub.
    // the size of which is expected to be less than 4K.
    vm::bootmm_init(end + multiboot::BOOT_INFO_MAX_EXPECTED_SIZE,
                    (void *)P2V(4 * 1024 * 1024));

    // process the multiboot information
    multiboot::init_mbi();
    multiboot::parse_multiboot_tags();

    // initialize the paging
    vm::init_kernelvm();

    // acpi initialization
    // acpi::acpi_init();

    console::printf("Hello world! build=%d\n", 5);

    console::console_settextattrib(console::TATTR_BKMAGENTA | console::TATTR_FRYELLOW);
    console::puts("colored text\n");

    console::console_settextattrib(console::TATTR_BKBLACK | console::TATTR_FRLTGRAY);
    console::puts("noncolored text\n");

    int condition = 10;
    condition -= 100;

    for (;;)
        ;
}

//C++ ctors and dtors
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

    void call_dtors(void)
    {
        //TODO : call global desturctors
    }
}
