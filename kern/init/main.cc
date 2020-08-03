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
#include "system/process.h"
#include "system/scheduler.h"
#include "system/syscall.h"
#include "system/vmm.h"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>



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

	// initialize I/O APIC
	io_apic::init_ioapic();

	// initialize syscall
	syscall::system_call_init();

	// initialize user process manager
	process::process_init();

	// boot other CPU cores
	ap::init_ap();

	write_format("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

	ap::all_processor_main();

	for (;;);
}
