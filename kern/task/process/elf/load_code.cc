#include "load_code.hpp"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "internals/elf64_spec.hpp"

#include "../../../libs/basic_io/include/builtin_text_io.hpp"
#include "kbl/data/pod_list.h"

#include "task/process/process.hpp"

#include "memory/pmm.hpp"

#include <utility>

using namespace executable;

using namespace memory;

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
	IN task::process* proc)
{
	auto[vm_flags, perms] = parse_ph_flags(prog_header);

//	auto proc_mm = proc->get_mm();
	auto as = proc->address_space();

//	if ((ret = vmm::mm_map(proc_mm, prog_header.p_vaddr, prog_header.p_memsz, vm_flags, nullptr)) != ERROR_SUCCESS)
//	{
//		//TODO do clean-ups
//		return ret;
//	}


	auto map_ret = as->map(prog_header.p_vaddr, prog_header.p_memsz, vm_flags);
	if (has_error(map_ret))
	{
		return get_error_code(map_ret);
	}


//	if (proc_mm->brk_start < prog_header.p_vaddr + prog_header.p_memsz)
//	{
//		proc_mm->brk_start = prog_header.p_vaddr + prog_header.p_memsz;
//	}

	if (as->heap_begin() < prog_header.p_vaddr + prog_header.p_memsz)
	{
		as->set_heap_begin(prog_header.p_vaddr + prog_header.p_memsz);
	}

	// ph->p_filesz <= ph->p_memsz
	size_t page_count = PAGE_ROUNDUP(prog_header.p_memsz) / PAGE_SIZE;

	auto alloc_ret =
		physical_memory_manager::instance()->allocate(prog_header.p_vaddr,
			page_count,
			perms,
			as->pgdir(),
			false);

	if (has_error(alloc_ret))
	{
		return get_error_code(alloc_ret);
	}

	auto pages = get_result(alloc_ret);
	memset((uint8_t*)pmm::page_to_va(pages), 0, page_count * PAGE_SIZE);
	memmove((uint8_t*)pmm::page_to_va(pages), bin + prog_header.p_offset, prog_header.p_filesz);

	return ERROR_SUCCESS;
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
	IN task::process* proc)
{
	error_code ret = ERROR_SUCCESS;

	auto[vm_flags, perms] = parse_sh_flags(shdr);
//	auto proc_mm = proc->get_mm();

//	if ((ret = vmm::mm_map(proc_mm, shdr.sh_addr, shdr.sh_size, vm_flags, nullptr)) != ERROR_SUCCESS)
//	{
//		//TODO do clean-ups
//		return ret;
//	}

	{
		auto map_ret = proc->address_space()->map(shdr.sh_addr, shdr.sh_size, vm_flags);
		if (has_error(map_ret))
		{
			return ret;
		}
	}

//	if (proc_mm->brk_start < shdr.sh_addr + shdr.sh_size)
//	{
//		proc_mm->brk_start = shdr.sh_addr + shdr.sh_size;
//	}
	if (proc->address_space()->heap_begin() < shdr.sh_addr + shdr.sh_size)
	{
		proc->address_space()->set_heap_begin(shdr.sh_addr + shdr.sh_size);
	}

	size_t page_count = PAGE_ROUNDUP(shdr.sh_size) / PAGE_SIZE;

//	auto alloc_ret =
//		physical_memory_manager::instance()->allocate(shdr.sh_addr, page_count, perms, proc_mm->pgdir, true);
	auto alloc_ret =
		physical_memory_manager::instance()->allocate(shdr.sh_addr,
			page_count,
			perms,
			proc->address_space()->pgdir(),
			true);

	if (has_error(alloc_ret))
	{
		return get_error_code(alloc_ret);
	}

	// set to zero
	memset((uint8_t*)pmm::page_to_va(get_result(alloc_ret)), 0, page_count * PAGE_SIZE);

	return ret;
}

error_code load_elf_binary(IN task::process* proc,
	const elf_executable& elf)
{
	Elf64_Phdr* prog_header = nullptr;
	size_t count = 0;
//	auto proc_mm = proc->get_mm();

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

	proc->address_space()->set_heap_begin(PAGE_ROUNDUP(proc->address_space()->heap_begin()));
	proc->address_space()->set_heap(PAGE_ROUNDUP(proc->address_space()->heap_begin()));

//	proc_mm->brk_start = proc_mm->brk = PAGE_ROUNDUP(proc_mm->brk_start);

	return ERROR_SUCCESS;
}

error_code load_binary(IN task::process* proc,
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
