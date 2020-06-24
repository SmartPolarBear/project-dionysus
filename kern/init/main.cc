
/*
 * Last Modified: Sun May 17 2020
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
#include "drivers/apic/timer.h"
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

void run_hello()
{
	uint8_t* bin = nullptr;
	size_t size = 0;

	auto ret = multiboot::find_module_by_cmdline("/hello", &size, &bin);

	KDEBUG_ASSERT(ret == ERROR_SUCCESS);

	process::process_dispatcher* proc_he = nullptr;
	process::create_process("hello", 0, false, &proc_he);

	KDEBUG_ASSERT(proc_he != nullptr);

	process::process_load_binary(proc_he, bin, size, process::BINARY_ELF);

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

	// initialize apic timer
	timer::init_apic_timer();

	// timer interrupt is only processed in cpu 0
	timer::set_enable_on_cpu(0, true);

	// initialize I/O APIC
	io_apic::init_ioapic();

	syscall::system_call_init();

	// initialize user process manager
	process::process_init();

	// boot other CPU cores
	ap::init_ap();

	write_format("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

	run_hello();

	scheduler::scheduler_yield();

	ap::all_processor_main();

	for (;;);
}
