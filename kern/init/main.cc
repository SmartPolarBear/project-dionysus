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
#include "drivers/monitor/monitor.hpp"
#include "drivers/simd/simd.hpp"
#include "drivers/pci/pci.hpp"
#include "drivers/cmos/rtc.hpp"

#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/multiboot.h"
#include "system/param.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/scheduler.h"
#include "system/syscall.h"
#include "system/vmm.h"

#include "fs/fs.hpp"

#include "../libs/basic_io/include/builtin_text_io.hpp"

#include "data/basic_containers.hpp"

// std::optional is usable unconditionally
#include <optional>

// std::any is usable with the pseudo-syscalls
#include <any>

// std::variant is usable with the pseudo-syscalls
#include <variant>

// std::span is usable unconditionally
#include <span>

#include "vector.hpp"

static inline void run(char* name)
{
	uint8_t* bin = nullptr;
	size_t size = 0;

	auto ret = multiboot::find_module_by_cmdline(name, &size, &bin);

	KDEBUG_ASSERT(ret == ERROR_SUCCESS);

	process::process_dispatcher* proc_he = nullptr;
	process::create_process(name, 0, false, &proc_he);

	KDEBUG_ASSERT(proc_he != nullptr);

	process::process_load_binary(proc_he, bin, size,
		process::BINARY_ELF,
		process::LOAD_BINARY_RUN_IMMEDIATELY);

	write_format("[cpu %d]load binary: %s, pid %d\n", cpu()->id, name, proc_he->id);
}

static inline void init_servers()
{
	// start monitor servers
	monitor::monitor_init();
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

	// initialize rtc to acquire date and time
	cmos::cmos_rtc_init();

	// initialize I/O APIC
	io_apic::init_ioapic();

	// initialize syscall
	syscall::system_call_init();

	// initialize SIMD like AVX and sse
	simd::enable_simd();

	// initialize PCI and PCIe
	pci::pci_init();

	// initialize the file system
	file_system::fs_init();

	// initialize user process manager
	process::process_init();

	// boot other CPU cores
	ap::init_ap();

	write_format("Codename \"dionysus\" built on %s %s\n", __DATE__, __TIME__);

	run("/ipctest");
	run("/hello");

	// start kernel servers in user space
	init_servers();

	ap::all_processor_main();

	for (;;);
}

void ap::all_processor_main()
{
	xchg(&cpu->started, 1u);

	// enable timer interrupt
	timer::set_enable_on_cpu(cpu->id, true);

	scheduler::scheduler_loop();
}
