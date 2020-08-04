#include "arch/amd64/x86.h"

#include "drivers/debug/kdebug.h"
#include "drivers/monitor/monitor.hpp"
#include "drivers/console/console.h"

#include "system/memlayout.h"
#include "system/types.h"
#include "system/process.h"
#include "system/multiboot.h"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>

console::console_dev g_monitor_dev;

static inline error_code load_monitor_executable()
{
	uint8_t* bin = nullptr;
	size_t size = 0;

	const char* name = "/monitor";

	auto ret = multiboot::find_module_by_cmdline(name, &size, &bin);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	process::process_dispatcher* proc_he = nullptr;
	ret = process::create_process(name, process::PROC_SYS_SERVER, false, &proc_he);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	ret = process::process_load_binary(proc_he, bin, size, process::BINARY_ELF);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code monitor::monitor_init()
{
	auto ret = load_monitor_executable();

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}
