/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-16 15:13:51
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
#include "lib/libkern/data/list.h"

#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/vm.h"

extern char end[]; // kernel.ld

struct teststruct
{
    char *name;
    int val;
    list_head node;
};

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

    // initialize the console
    console::console_init();

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

    // list_head head;
    // klib::list_init(&head);

    // teststruct t1 = {.name = "test1", .val = 1};
    // teststruct t2 = {.name = "test2", .val = 11};
    // teststruct t3 = {.name = "test3", .val = 111};

    // klib::list_add(&t1.node, &head);
    // klib::list_add(&t2.node, &head);
    // klib::list_add(&t3.node, &head);

    // klib::list_for_each(&head, [](list_head *h) {
    //     teststruct *element = list_entry(h, teststruct, node);
    //     console::printf("name=%s,val=%d\n", element->name, element->val);
    // });

    console::printf("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

    // boot other CPU cores
    ap::init_ap();

    // TODO: after a proper scheduler implementation, this should be the last line of main
    ap::all_processor_main();

    //TODO: implement process manager.

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
