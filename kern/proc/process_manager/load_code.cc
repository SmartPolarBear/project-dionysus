#include "load_code.hpp"

#include "system/error.h"
#include "system/kmalloc.h"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"

#include "elf/elf64_spec.hpp"

#include "libkernel/console/builtin_console.hpp"
#include "libkernel/data/List.h"

#include <utility>

using namespace executable;

static inline auto parse_ph_flags(const Elf64_Phdr& prog_header)
{
	size_t vm_flags = 0, perms = PG_U;

	if (prog_header.p_type & PF_X)
	{
		vm_flags |= vmm::VM_EXEC;
	}

	if (prog_header.p_type & PF_R)
	{
		vm_flags |= vmm::VM_READ;
	}

	if (prog_header.p_type & PF_W)
	{
		vm_flags |= vmm::VM_WRITE;
		perms |= PG_W;
	}

	return std::make_pair(vm_flags, perms);
}

static error_code load_ph(IN const Elf64_Phdr& prog_header,
	IN const uint8_t* bin,
	IN process::process_dispatcher* proc)
{
	error_code ret = ERROR_SUCCESS;

	auto[vm_flags, perms] = parse_ph_flags(prog_header);

	if ((ret = vmm::mm_map(proc->mm, prog_header.p_vaddr, prog_header.p_memsz, vm_flags, nullptr)) != ERROR_SUCCESS)
	{
		//TODO do clean-ups
		return ret;
	}

	if (proc->mm->brk_start < prog_header.p_vaddr + prog_header.p_memsz)
	{
		proc->mm->brk_start = prog_header.p_vaddr + prog_header.p_memsz;
	}

	// ph->p_filesz <= ph->p_memsz
	size_t page_count = PAGE_ROUNDUP(prog_header.p_memsz) / PAGE_SIZE;
	page_info* pages = nullptr;
	auto error = pmm::pgdir_alloc_pages(proc->mm->pgdir, false, page_count, prog_header.p_vaddr, perms, &pages);

	if (error != ERROR_SUCCESS)
	{
		return error;
	}

	memset((uint8_t*)pmm::page_to_va(pages), 0, page_count * PAGE_SIZE);
	memmove((uint8_t*)pmm::page_to_va(pages), bin + prog_header.p_offset, prog_header.p_filesz);

	return ret;
}

static inline auto parse_sh_flags(const Elf64_Shdr& shdr)
{
	size_t vm_flags = vmm::VM_READ, perms = PG_U;

	if (shdr.sh_flags & SHF_EXECINSTR)
	{
		vm_flags |= vmm::VM_EXEC;
	}

	if (shdr.sh_flags & SHF_WRITE)
	{
		vm_flags |= vmm::VM_WRITE;
		perms |= PG_W;
	}

	return std::make_pair(vm_flags, perms);
}

static error_code alloc_sh(IN const Elf64_Shdr& shdr,
	IN  [[maybe_unused]] uint8_t* bin,
	IN process::process_dispatcher* proc)
{
	error_code ret = ERROR_SUCCESS;

	auto[vm_flags, perms] = parse_sh_flags(shdr);

	if ((ret = vmm::mm_map(proc->mm, shdr.sh_addr, shdr.sh_size, vm_flags, nullptr)) != ERROR_SUCCESS)
	{
		//TODO do clean-ups
		return ret;
	}

	if (proc->mm->brk_start < shdr.sh_addr + shdr.sh_size)
	{
		proc->mm->brk_start = shdr.sh_addr + shdr.sh_size;
	}

	size_t page_count = PAGE_ROUNDUP(shdr.sh_size) / PAGE_SIZE;
	page_info* pages = nullptr;
	auto error = pmm::pgdir_alloc_pages(proc->mm->pgdir, true, page_count, shdr.sh_addr, perms, &pages);

	if (error != ERROR_SUCCESS)
	{
		return error;
	}

	// set to zero
	memset((uint8_t*)pmm::page_to_va(pages), 0, page_count * PAGE_SIZE);

	return ret;
}

error_code load_elf_binary(IN process::process_dispatcher* proc,
	const elf_executable& elf)
{
	Elf64_Phdr* prog_header = nullptr;
	size_t count = 0;

	auto ret = elf.get_program_headers(&prog_header, &count);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	auto data = elf.get_data();

	for (size_t i = 0; i < count; i++)
	{
		if (prog_header[i].p_type == PT_LOAD)
		{
			ret = load_ph(prog_header[i], (uint8_t*)data, proc);

			if (ret != ERROR_SUCCESS)
			{
				break;
			}
		}
	}

	Elf64_Shdr* section_headers = nullptr;
	elf.get_section_headers(&section_headers, &count);
	for (size_t i = 0; i < count; i++)
	{
		// allocate
		if (section_headers[i].sh_type == SHT_NOBITS &&
			section_headers[i].sh_size != 0 &&
			section_headers[i].sh_flags & SHF_ALLOC)
		{
			if ((ret = alloc_sh(section_headers[i], (uint8_t*)data, proc)) != ERROR_SUCCESS)
			{
				return ret;
			}
		}
	}

	proc->mm->brk_start = proc->mm->brk = PAGE_ROUNDUP(proc->mm->brk_start);

	return ERROR_SUCCESS;
}

error_code load_binary(IN process::process_dispatcher* proc,
	IN uint8_t* bin,
	IN size_t bin_sz,
	OUT uintptr_t* entry_addr)
{
	elf_executable elf{};
	error_code ret = elf.parse(binary{ .size=bin_sz, .data=bin });

	if (ret == ERROR_SUCCESS)
	{
		Elf64_Ehdr* elf_header = nullptr;
		elf.get_elf_header(&elf_header);
		*entry_addr = elf_header->e_entry;

		return load_elf_binary(proc, elf);
	}

	return -ERROR_INVALID;
}
