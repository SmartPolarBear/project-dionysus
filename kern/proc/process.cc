/*
 * Last Modified: Mon Mar 23 2020
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

#include "process.hpp"

#include "sys/error.h"
#include "sys/kmalloc.h"
#include "sys/memlayout.h"
#include "sys/pmm.h"
#include "sys/proc.h"
#include "sys/vmm.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/stdio.h"
#include "lib/libelf/elf.h"
#include "lib/libkern/data/list.h"

using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

using elf::elfhdr;
using elf::proghdr;

__thread process::process_dispatcher *current;

// scheduler.cc
void scheduler_ret();

process_list_struct proc_list;

static inline error_code elf_load_binary(IN process::process_dispatcher *proc,
										 IN uint8_t *bin)
{
	error_code ret = ERROR_SUCCESS;

	elf::elfhdr *header = (elfhdr *)bin;
	if (header->magic != elf::ELF_MAGIC)
	{
		return ERROR_INVALID_ARG;
	}

	proghdr *prog_header = (proghdr *)(bin + header->phoff);

	for (size_t i = 0; i < header->phnum; i++)
	{
		if (prog_header[i].type == elf::ELF_PROG_LOAD)
		{
			size_t vm_flags = 0, perms = PG_U;
			if (prog_header[i].type & elf::ELF_PROG_FLAG_EXEC)
			{
				vm_flags |= vmm::VM_EXEC;
			}

			if (prog_header[i].type & elf::ELF_PROG_FLAG_READ)
			{
				vm_flags |= vmm::VM_READ;
			}

			if (prog_header[i].type & elf::ELF_PROG_FLAG_WRITE)
			{
				vm_flags |= vmm::VM_WRITE;
				perms |= PG_W;
			}

			if ((ret = vmm::mm_map(proc->mm, prog_header[i].vaddr, prog_header[i].memsz, vm_flags, nullptr)) != ERROR_SUCCESS)
			{
				//TODO do clean-ups
				return ret;
			}

			if (proc->mm->brk_start < prog_header[i].vaddr + prog_header[i].memsz)
			{
				proc->mm->brk_start = prog_header[i].vaddr + prog_header[i].memsz;
			}

			uintptr_t start = prog_header[i].vaddr, end = prog_header[i].vaddr + prog_header[i].filesz;
			uintptr_t la = rounddown(start, PAGE_SIZE);
			uintptr_t offset = prog_header[i].off;
			page_info *page = nullptr;

			while (start < end)
			{
				page = pmm::pgdir_alloc_page(proc->mm->pgdir, la, perms);
				if (page == nullptr)
				{
					//TODO do clean-ups
					return -ERROR_MEMORY_ALLOC;
				}

				size_t off = start - la, size = PAGE_SIZE - off;
				la += PAGE_SIZE;
				if (end < la)
				{
					size -= la - end;
				}

				memmove(((uint8_t *)pmm::page_to_va(page)) + off, bin + offset, size);
				start += size, offset += size;
			}

			end = prog_header[i].vaddr + prog_header[i].memsz;

			if (start < la)
			{
				if (start == end)
				{
					continue;
				}
				size_t off = start + PAGE_SIZE - la, size = PAGE_SIZE - off;
				if (end < la)
				{
					size -= la - end;
				}
				// memset(((uint8_t *)pmm::page_to_va(page)) + off, 0, size);
				start += size;
				if ((end < la && start == end) || (end >= la && start == la))
				{
					//TODO do clean-ups
					return -ERROR_INVALID_DATA;
				}
			}

			// while (start < end)
			// {
			// 	page = pmm::pgdir_alloc_page(proc->mm->pgdir, la, perms);
			// 	if (page == NULL)
			// 	{
			// 		//TODO do clean-ups
			// 		return -ERROR_MEMORY_ALLOC;
			// 	}
			// 	size_t off = start - la, size = PAGE_SIZE - off;

			// 	la += PAGE_SIZE;
			// 	if (end < la)
			// 	{
			// 		size -= la - end;
			// 	}
			// 	// memset(((uint8_t *)pmm::page_to_va(page)) + off, 0, size);
			// 	start += size;
			// }
		}
		else
		{
			//TODO: handle more header types
		}
	}

	proc->trapframe.rip = header->entry;

	// allocate an stack
	for (size_t i = 0; i < process::process_dispatcher::KERNSTACK_PAGES; i++)
	{
		uintptr_t va = USER_TOP - process::process_dispatcher::KERNSTACK_SIZE + i * PAGE_SIZE;
		pmm::pgdir_alloc_page(proc->mm->pgdir, va, PG_W | PG_U | PG_PS | PG_P);
	}

	return ret;
}

// precondition: the lock must be held
static inline process::pid alloc_pid(void)
{
	return proc_list.proc_count++;
}

static inline error_code setup_mm(process::process_dispatcher *proc)
{
	proc->mm = vmm::mm_create();
	if (proc->mm == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	vmm::pde_ptr_t pgdir = vmm::pgdir_entry_alloc();

	if (pgdir == nullptr)
	{
		vmm::mm_destroy(proc->mm);
		return -ERROR_MEMORY_ALLOC;
	}

	memmove(pgdir, g_kpml4t, PGTABLE_SIZE);

	proc->mm->pgdir = pgdir;

	return ERROR_SUCCESS;
}

void process::process_init(void)
{
	proc_list.proc_count = 0;
	spinlock_initlock(&proc_list.lock, "proc");
	list_init(&proc_list.head);
}

error_code process::create_process(IN const char *name,
								   IN size_t flags,
								   IN bool inherit_parent,
								   OUT process_dispatcher **retproc)
{
	spinlock_acquire(&proc_list.lock);

	if (proc_list.proc_count >= process::PROC_MAX_COUNT)
	{
		return -ERROR_TOO_MANY_PROC;
	}

	auto proc = new process_dispatcher(name, alloc_pid(), current == nullptr ? 0 : current->id, flags);

	//setup kernel stack
	proc->kstack = (uintptr_t) new BLOCK<process::process_dispatcher::KERNSTACK_SIZE>;

	if (!proc->kstack)
	{
		delete proc;
		return -ERROR_MEMORY_ALLOC;
	}

	error_code ret = ERROR_SUCCESS;
	if ((ret = setup_mm(proc)) != ERROR_SUCCESS)
	{
		delete proc;
		return ret;
	}

	proc->trapframe.cs = (SEG_UCODE << 3) | DPL_USER;
	proc->trapframe.ss = (SEG_UDATA << 3) | DPL_USER;
	proc->trapframe.rsp = USER_STACK_TOP;

	proc->trapframe.rflags |= EFLAG_IF;

	if (flags & PROC_SYS_SERVER)
	{
		proc->trapframe.rflags |= EFLAG_IOPL_MASK;
	}

	if (inherit_parent)
	{
		// TODO: copy data from parent process
	}

	list_add(&proc->link, &proc_list.head);

	spinlock_release(&proc_list.lock);

	*retproc = proc;
	return ERROR_SUCCESS;
}

error_code process::process_load_binary(IN process_dispatcher *proc,
										IN uint8_t *bin,
										[[maybe_unused]] IN size_t binsize OPTIONAL,
										IN binary_types type)
{
	error_code ret = ERROR_SUCCESS;
	if (type == BINARY_ELF)
	{
		ret = elf_load_binary(proc, bin);
	}
	else
	{
		ret = ERROR_INVALID_ADDR;
	}

	if (ret == ERROR_SUCCESS)
	{
		proc->state = PROC_STATE_RUNNABLE;
	}

	return ret;
}

void do_iret(trap_frame tf)
{
	asm volatile(
		"\tmovq %0,%%rsp\n"
		"\tpopq %%rax\n"
		"\tpopq %%rbx\n"
		"\tpopq %%rcx\n"
		"\tpopq %%rdx\n"
		"\tpopq %%rbp\n"
		"\tpopq %%rsi\n"
		"\tpopq %%rdi\n"
		"\tpopq %%r8 \n"
		"\tpopq %%r9 \n"
		"\tpopq %%r10\n"
		"\tpopq %%r11\n"
		"\tpopq %%r12\n"
		"\tpopq %%r13\n"
		"\tpopq %%r14\n"
		"\tpopq %%r15\n"
		"\taddq $16, %%rsp\n" //discard trapnum and errorcode
		"\tiretq\n" ::"g"(&tf)
		: "memory");

	KDEBUG_GENERALPANIC("iretq failed.");
}

error_code process::process_run(IN process_dispatcher *proc)
{

	if (current != nullptr && current->state == PROC_STATE_RUNNING)
	{
		current->state = PROC_STATE_RUNNABLE;
	}

	trap::pushcli();

	current = proc;
	current->state = PROC_STATE_RUNNING;
	current->runs++;

	KDEBUG_ASSERT(current != nullptr && current->mm != nullptr);

	lcr3(V2P((uintptr_t)current->mm->pgdir));

	spinlock_release(&proc_list.lock);

	vmm::tss_set_rsp(cpu->get_tss(), 0, current->kstack + process_dispatcher::KERNSTACK_SIZE);

	trap::popcli();

	// proc_restore_trapframe(&current->trapframe);
	do_iret(current->trapframe);

	KDEBUG_FOLLOWPANIC("iret failed");
}