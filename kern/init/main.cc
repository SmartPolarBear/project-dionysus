/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-01 12:35:56
 * @ Description: the entry point for kernel in C++
 */

#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic_timer/timer.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"

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
                    (void *)P2V(32_MB));

    // process the multiboot information
    multiboot::init_mbi();

    // initialize the paging
    vm::init_kernelvm();

    // initialize ACPI
    acpi::init_acpi();

    // initialize local APIC
    local_apic::init_lapic();

    // initialize apic timer
    timer::init_apic_timer();
    
    // install trap vectors
    trap::initialize_trap_vectors();

    // install gdt
    vm::segment::init_segmentation();

    // initialize I/O APIC
    io_apic::init_ioapic();

    // boot other CPU cores
    ap::init_ap();

    // TODO: after a proper scheduler implementation, this should be the last line of main
    ap::all_processor_main();

    //TODO: implement process manager.
    


    console::printf("Codename \"dionysus\" built on %s %s\nBoot OK!\n", __DATE__, __TIME__);

    for (;;)
        ;
}

// C++ ctors and dtors
// usually called from boot.S
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
