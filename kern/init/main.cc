/*
 * Last Modified: Thu Feb 20 2020
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
#include "drivers/debug/kerror.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"
#include "lib/libkern/data/list.h"

#include "sys/kmalloc.h"
#include "sys/memlayout.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/pmm.h"
#include "sys/vmm.h"

#include "lib/libc/stdio.h"

// global entry of the kernel
extern "C" [[noreturn]] void kmain() {
    // process the multiboot information
    multiboot::init_mbi();

    // initialize physical memory
    pmm::init_pmm();

    // install trap vectors nad handle structures
    trap::init_trap();

    // initialize the console
    console::console_init();

    // initialize ACPI
    acpi::init_acpi();

    // initialize local APIC
    local_apic::init_lapic();

    // initialize I/O APIC
    io_apic::init_ioapic();

    // initialize apic timer
    timer::init_apic_timer();

    printf("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

    // boot other CPU cores
    ap::init_ap();

    // TODO: implement process manager.
    // TODO: after a proper scheduler implementation, this should be the last line of main
    ap::all_processor_main();

    for (;;)
        ;
}

// C++ ctors and dtors
// usually called from boot.S
extern "C"
{
    using ctor_type = void (*)();
    using dtor_type = void (*)();

    extern ctor_type start_ctors;
    extern ctor_type end_ctors;

    extern dtor_type start_dtors;
    extern dtor_type end_dtors;

    // IMPORTANT: initialization of libc components such as printf depends on this.
    void call_ctors(void)
    {
        for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
        {
            (*ctor)();
        }
    }

    void call_dtors(void)
    {
        for (auto dtor = &start_dtors; dtor != &end_dtors; dtor++)
        {
            (*dtor)();
        }
    }
}
