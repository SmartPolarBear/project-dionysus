#include "arch/amd64/x86.h"

#include "drivers/debug/kdebug.h"
#include "drivers/monitor/monitor.hpp"
#include "drivers/monitor/vbe.hpp"
#include "drivers/console/console.h"

#include "system/memlayout.h"
#include "system/types.h"
#include "system/process.h"
#include "system/multiboot.h"
#include "system/pmm.h"

#include "libkernel/console/builtin_text_io.hpp"

console::console_dev g_monitor_dev{};
process::process_dispatcher* g_monitor_proc = nullptr;

static inline error_code map_framebuffer(multiboot_tag_framebuffer* framebuffer_tag)
{
	vmm::mm_map(g_monitor_proc->mm, framebuffer_tag->common.framebuffer_addr,
		PAGE_SIZE, vmm::VM_WRITE, nullptr);

	auto error = vmm::map_range(g_monitor_proc->mm->pgdir,
		framebuffer_tag->common.framebuffer_addr,
		framebuffer_tag->common.framebuffer_addr,
		PAGE_SIZE);

	if (error != ERROR_SUCCESS)
	{
		return error;
	}

	return ERROR_SUCCESS;
}

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

	ret = process::create_process(name, process::PROC_SYS_SERVER, false, &g_monitor_proc);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	// load the process, but not run
	ret = process::process_load_binary(g_monitor_proc, bin, size, process::BINARY_ELF, 0);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

static inline error_code load_server_info()
{
	constexpr size_t info_size =
		sizeof(multiboot_tag_vbe) +
			sizeof(multiboot_tag_framebuffer) +
			sizeof(uintptr_t[2]);

	// allocate more program heap memory
	size_t page_count = PAGE_ROUNDUP(info_size) / PAGE_SIZE;
	page_info* pages = nullptr;
	auto ret = pmm::pgdir_alloc_pages(g_monitor_proc->mm->pgdir,
		true,
		page_count,
		g_monitor_proc->mm->brk_start,
		PG_U | PG_P,
		&pages);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	// load multiboot tags
	auto framebuffer_tag = multiboot::acquire_tag_ptr<multiboot_tag_framebuffer>(MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

	if (framebuffer_tag == nullptr)
	{
		return -ERROR_VMA_NOT_FOUND;
	}

	// map a page for framebuffer
	ret = map_framebuffer(framebuffer_tag);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	uintptr_t heap_addr = pmm::page_to_va(pages);

	// copy framebuffer tag to server's heap
	uintptr_t frmbuf_tag_user = heap_addr;
	heap_addr += sizeof(*framebuffer_tag);
	memmove((void*)frmbuf_tag_user, framebuffer_tag, sizeof(*framebuffer_tag));

	// the argv array
	uintptr_t argv_start = heap_addr, argv_ptr = heap_addr;
	argv_ptr += sizeof(uintptr_t); // argv[0] should be the path to program itself, now we leave it empty
	*((uintptr_t*)argv_ptr) = frmbuf_tag_user;
	argv_ptr += sizeof(uintptr_t);

	heap_addr = argv_ptr;

	g_monitor_proc->tf->rdi = 2; // argc
	g_monitor_proc->tf->rsi = argv_start; // argv

	return ERROR_SUCCESS;
}

error_code monitor::monitor_init()
{

	auto ret = load_monitor_executable();

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	ret = load_server_info();

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	// process is ready to run
	g_monitor_proc->mm->brk_start = g_monitor_proc->mm->brk = PAGE_ROUNDUP(g_monitor_proc->mm->brk_start);
	g_monitor_proc->state = process::PROC_STATE_RUNNABLE;

	return ERROR_SUCCESS;
}
