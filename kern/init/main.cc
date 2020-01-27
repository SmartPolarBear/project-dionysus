/*
 * Last Modified: Mon Jan 27 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
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

#include "sys/memlayout.h"
#include "sys/mm.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/vm.h"

struct teststruct
{
    char a;
    int b;
    size_t c;
    int arr[100];
};

// global entry of the kernel
extern "C" [[noreturn]] void kmain()
{
    // memory allocator at boot time, allocating memory within (end+0x1000,P2V(4MB))
    // the offset is intended to protect multiboot info from overwritten,
    // which is put *right* after the kernel by grub.
    // the size of which is expected to be less than 4K.
    vm::bootmm_init(vm::kernel_boot_mem_begin(),
                    vm::kernel_boot_mem_end());

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

    // initialize buddy allocator
    vm::buddy_init(vm::kernel_mem_begin(), vm::kernel_mem_end());

    for (int i = 1; i <= 24; i++)
    {
        teststruct *ts[10];
        for (int j = 0; j < 10; j++)
        {
            ts[j] = (teststruct *)vm::buddy_alloc(sizeof(teststruct));
        }

        for (int j = 0; j < 10; j++)
        {
            teststruct *t = ts[j];
            t->a = 12 * i + j;
            t->b = 123 * i + j;
            t->c = 0x3f3f3f * i + j;
            for (int k = 0; k < 100; k++)
                t->arr[k] = k * i + j;
        }

        for (int j = 9; j >= 0; j--)
        {
            teststruct *t = ts[j];
            console::printf("a,b,c=%d %d %lld\n", t->a, t->b, t->c);
            for (int k = 0; k < 100; k++)
            {
                console::printf("arr[%d]=%d\n", k, t->arr[k]);
            }

            vm::buddy_free(t);
            console::printf("========\n");
        }
        console::printf("-----------\n");
    }

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
