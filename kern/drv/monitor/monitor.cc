#include "arch/amd64/x86.h"

#include "drivers/debug/kdebug.h"
#include "drivers/monitor/monitor.hpp"
#include "drivers/console/console.h"

#include "system/memlayout.h"
#include "system/types.h"
#include "system/process.h"
#include "system/multiboot.h"

#include <cstring>

console::console_dev g_monitor_dev;

static inline void load_monitor_binary()
{
	uint8_t* bin = nullptr;
	size_t size = 0;

	const char* monitor_name = "/monitor";

	// find multiboot info for monitor executable
	auto ret = multiboot::find_module_by_cmdline(monitor_name, &size, &bin);

	KDEBUG_ASSERT(ret == ERROR_SUCCESS);

	process::process_dispatcher* proc_he = nullptr;
	process::create_process(monitor_name, 0, false, &proc_he);

	KDEBUG_ASSERT(proc_he != nullptr);

	process::process_load_binary(proc_he, bin, size, process::BINARY_ELF);

}

error_code monitor::monitor_init()
{
	load_monitor_binary();

	console::console_remove_internal_devs();

	console::console_add_dev(&g_monitor_dev);
}
