/*
 * Last Modified: Wed Feb 12 2020
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

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/apic_timer/timer.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "sys/kmalloc.h"
#include "sys/memlayout.h"
#include "sys/multiboot.h"
#include "sys/pmm.h"
#include "sys/types.h"
#include "sys/vmm.h"

#include "lib/libc/string.h"

// in boot.S
extern "C" void entry32mp(void);

constexpr uintptr_t AP_CODE_LOAD_ADDR = 0x7000;

[[clang::optnone]] void ap::init_ap(void)
{

    auto tag = multiboot::acquire_tag_ptr<multiboot_tag_module>(MULTIBOOT_TAG_TYPE_MODULE,
                                                                [](auto ptr) -> bool {
                                                                    multiboot_tag_module *mdl_tag = reinterpret_cast<decltype(mdl_tag)>(ptr);
                                                                    const char *ap_boot_commandline = "/ap_boot";
                                                                    auto cmp = strncmp(mdl_tag->cmdline, ap_boot_commandline, strlen(ap_boot_commandline));
                                                                    return cmp == 0;
                                                                });
    KDEBUG_ASSERT(tag != nullptr);

    uint8_t *code = reinterpret_cast<decltype(code)>(P2V(AP_CODE_LOAD_ADDR));

    // Attention: not knowing tag->mod_end is the address of the last byte or next to it. May need +1 for code size
    //    Although with or without +1 it runs fine, bugs may occurs if the code changes.

    size_t code_size = tag->mod_end - tag->mod_start;
    memmove(code,
            reinterpret_cast<decltype(code)>(P2V_KERNEL(tag->mod_start)),
            code_size);

    for (const auto &core : cpus)
    {
        if (core.present && core.id != local_apic::get_cpunum())
        {
            uint8_t *stack = new BLOCK<PMM_PAGE_SIZE>;

            if (stack == nullptr)
            {
                KDEBUG_RICHPANIC("Can't allocate enough memory for AP boot.\n",
                                 "KERNEL PANIC: AP",
                                 false, "");
            }

            *(uint32_t *)(code - 4) = 0x8000; // just enough stack to get us to entry64mp
            *(uint32_t *)(code - 8) = V2P(uintptr_t(entry32mp));
            *(uint64_t *)(code - 16) = (uint64_t)(stack + PG_SIZE);

            local_apic::start_ap(core.apicid, V2P((uintptr_t)code));

            while (core.started == 0u)
                ;
        }
    }
}

void ap::all_processor_main()
{
    xchg(&cpu->started, 1u);

    //FIXME: temporarily enable interrupt
    sti();

    // simple scheduler loop
    while (!kdebug::panicked)
    {
        console::printf("cpu %d\n", cpu->id);
    }

    if (kdebug::panicked)
    {
        cli();
        hlt();
        for (;;)
            ;
    }
}

extern "C" [[clang::optnone]] void ap_enter(void)
{
    // install the kernel vm
    vmm::install_kpml4();

    // initialize segmentation
    vmm::install_gdt();

    // the calling sequence of the two functions above is not the same as the boot CPU
    // because we need paging enabled first.

    // install trap vectors
    trap::initialize_trap_vectors();

    // initialize local APIC
    local_apic::init_lapic();

    // initialize apic timer
    timer::init_apic_timer();

    ap::all_processor_main();
}
