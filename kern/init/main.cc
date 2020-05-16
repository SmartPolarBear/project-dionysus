
/*
 * Last Modified: Wed May 13 2020
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

#include "system/kmalloc.h"
#include "system/memlayout.h"
#include "system/multiboot.h"
#include "system/param.h"
#include "system/pmm.h"
#include "system/proc.h"
#include "system/scheduler.h"
#include "system/syscall.h"
#include "system/vmm.h"

#include "libraries/libkern/data/list.h"
#include "libraries/libkernel/console/builtin_console.hpp"

#include <cstring>
#include <utility>


void run_hello()
{
    auto tag = multiboot::acquire_tag_ptr<multiboot_tag_module>(MULTIBOOT_TAG_TYPE_MODULE, [](auto ptr) -> bool {
        multiboot_tag_module *mdl_tag = reinterpret_cast<decltype(mdl_tag)>(ptr);
        const char *ap_boot_commandline = "/hello";
        auto cmp = strncmp(mdl_tag->cmdline, ap_boot_commandline, strlen(ap_boot_commandline));
        return cmp == 0;
    });

    KDEBUG_ASSERT(tag != nullptr);

    process::process_dispatcher *proc_he = nullptr;
    process::create_process("hello", 0, false, &proc_he);
    KDEBUG_ASSERT(proc_he != nullptr);

    process::process_load_binary(proc_he, (uint8_t *)P2V(tag->mod_start), tag->mod_end - tag->mod_start + 1, process::BINARY_ELF);

    write_format("load binary: hello\n");
}

// global entry of the kernel
extern "C" [[noreturn]] void kmain()
{
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

    syscall::system_call_init();

    // initialize user process manager
    process::process_init();

    run_hello();
    
    write_format("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

    // boot other CPU cores
    ap::init_ap();

    scheduler::scheduler_yield();

    ap::all_processor_main();

    for (;;)
        ;
}
