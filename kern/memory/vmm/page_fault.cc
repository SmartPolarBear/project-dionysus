
// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "memory/pmm.hpp"
#include "memory/address_space.hpp"

#include "task/process/process.hpp"
#include "task/thread//thread.hpp"

#include <cstring>
#include <algorithm>

using namespace vmm;
using namespace memory;

static inline error_code page_fault_impl(size_t err, uintptr_t addr)
{
	auto proc = cur_proc.get();
	auto vma = proc->address_space()->find_vma(addr);
	if (vma == nullptr || vma->start() > addr)
	{
		return -ERROR_VMA_NOT_FOUND;
	}

	switch (err & 0b11)
	{
	default:
	case 0b10: // write, not persent
		if (!(vma->flags() & VM_WRITE))
		{
			return -ERROR_PAGE_NOT_PRESENT;
		}
		break;
	case 0b01: // read, persent
		return -ERROR_UNKOWN;
		break;
	case 0b00: // read not persent
		if (!(vma->flags() & (VM_READ | VM_EXEC)))
		{
			return -ERROR_PAGE_NOT_PRESENT;
		}
		break;
	}

	size_t page_perm = PG_U;
	if (vma->flags() & VM_WRITE)
	{
		page_perm |= PG_W;
	}

	addr = rounddown(addr, PAGE_SIZE);

	auto page_ret =
		memory::physical_memory_manager::instance()->allocate(addr, page_perm, proc->address_space()->pgdir(), true);
	if (has_error(page_ret)) // map to any free space
	{
		return get_error_code(page_ret);
	}

	return ERROR_SUCCESS;

}

error_code handle_pgfault([[maybe_unused]] trap::trap_frame info)
{
	uintptr_t addr = rcr2();

	if (!cur_proc.is_valid() || cur_proc == nullptr) // page fault from kernel
	{
		KDEBUG_RICHPANIC("!cur_proc.is_valid() || cur_proc == nullptr",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p, PC= 0x%p, thread name_ %s on CPU %d\n",
			addr,
			info.rip,
			task::cur_thread->get_name_raw(),
			cpu->id);
	}

	error_code ret = page_fault_impl(info.err, addr);

	if (ret == -ERROR_VMA_NOT_FOUND)
	{
		KDEBUG_RICHPANIC("The Addr isn't found in th MM structure.",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}
	else if (ret == -ERROR_PAGE_NOT_PRESENT)
	{
		KDEBUG_RICHPANIC("A page's not persent.",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}
	else if (ret == -ERROR_UNKOWN)
	{
		KDEBUG_RICHPANIC("Unkown error in paging",
			"KERNEL PANIC: PAGE FAULT",
			false,
			"Address: 0x%p\n", addr);
	}

	return ret;
}