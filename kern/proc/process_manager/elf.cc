/*
 * Last Modified: Tue Apr 21 2020
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

static inline auto get_vm_properties_for_header(proghdr prog_header)
{
    size_t vm_flags = 0, perms = PG_U;
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

[[clang::optnone]] static error_code load_section(IN proghdr prog_header,
                                                  IN const uint8_t *bin,
                                                  IN process::process_dispatcher *proc)
{
    error_code ret = ERROR_SUCCESS;

    auto [vm_flags, perms] = get_vm_properties_for_header(prog_header);

    if ((ret = vmm::mm_map(proc->mm, prog_header.vaddr, prog_header.memsz, vm_flags, nullptr)) != ERROR_SUCCESS)
    {
        //TODO do clean-ups
        return ret;
    }

    if (proc->mm->brk_start < prog_header.vaddr + prog_header.memsz)
    {
        proc->mm->brk_start = prog_header.vaddr + prog_header.memsz;
    }

    // ph->p_filesz <= ph->p_memsz
    size_t page_count = PAGE_ROUNDUP(prog_header.memsz) / PAGE_SIZE;
    page_info *pages = nullptr;
    auto error = pmm::pgdir_alloc_pages(proc->mm->pgdir, false, page_count, prog_header.vaddr, perms, &pages);

    if (error != ERROR_SUCCESS)
    {
        return error;
    }

    memset((uint8_t *)pmm::page_to_va(pages), 0, page_count * PAGE_SIZE);
    memmove((uint8_t *)pmm::page_to_va(pages), bin + prog_header.off, prog_header.filesz);

    return ret;
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
            ret = load_section(prog_header[i], bin, proc);

            if (ret != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    *entry_addr = header->entry;

    return ret;
}
