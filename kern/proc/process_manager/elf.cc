/*
 * Last Modified: Sun Apr 12 2020
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

#include "elf.hpp"

#include "sys/error.h"
#include "sys/kmalloc.h"
#include "sys/memlayout.h"
#include "sys/pmm.h"
#include "sys/proc.h"
#include "sys/vmm.h"

#include "lib/libc/stdio.h"
#include "lib/libcxx/utility"
#include "lib/libkern/data/list.h"

static inline auto get_vm_properties_for_header(const proghdr &prog_header)
{
    size_t vm_flags = 0, perms = 0;
    if (prog_header.type & ELF_PROG_FLAG_EXEC)
    {
        vm_flags |= vmm::VM_EXEC;
    }

    if (prog_header.type & ELF_PROG_FLAG_READ)
    {
        vm_flags |= vmm::VM_READ;
    }

    if (prog_header.type & ELF_PROG_FLAG_WRITE)
    {
        vm_flags |= vmm::VM_WRITE;
        perms |= PG_W;
    }

    return sysstd::value_pair<size_t, size_t>{vm_flags, perms};
}

error_code elf_load_binary(IN process::process_dispatcher *proc,
                           IN uint8_t *bin,
                           OUT uintptr_t *entry_addr)
{
    error_code ret = ERROR_SUCCESS;

    elfhdr *header = (elfhdr *)bin;
    if (header->magic != ELF_MAGIC)
    {
        return ERROR_INVALID_ARG;
    }

    proghdr *prog_header = (proghdr *)(bin + header->phoff);

    for (size_t i = 0; i < header->phnum; i++)
    {
        if (prog_header[i].type == ELF_PROG_LOAD)
        {
            auto [vm_flags, perms] = get_vm_properties_for_header(prog_header[i]);

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

    *entry_addr = header->entry;
    // proc->trapframe.rip = header->entry;

    // // allocate an stack
    // for (size_t i = 0; i < process::process_dispatcher::KERNSTACK_PAGES; i++)
    // {
    //     uintptr_t va = USER_TOP - process::process_dispatcher::KERNSTACK_SIZE + i * PAGE_SIZE;
    //     pmm::pgdir_alloc_page(proc->mm->pgdir, va, PG_W | PG_U | PG_PS | PG_P);
    // }

    return ret;
}
