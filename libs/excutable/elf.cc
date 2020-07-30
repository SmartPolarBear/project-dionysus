#include "elf.hpp"
#include "system/error.h"

using namespace executable;

error_code elf_executable::parse(binary _bin)
{
	this->bin = _bin;

	elf_header = (Elf64_Ehdr*)bin.data;

	if (elf_header->e_ident[EI_MAG0] != 0x7F ||
		elf_header->e_ident[EI_MAG1] != 'E' ||
		elf_header->e_ident[EI_MAG2] != 'L' ||
		elf_header->e_ident[EI_MAG3] != 'F')
	{
		m_valid = false;
		return -ERROR_INVALID_DATA;
	}

	m_valid = true;

	return ERROR_SUCCESS;
}

error_code elf_executable::get_elf_header(OUT Elf64_Ehdr* out)
{
	if (!m_valid)
	{
		return -ERROR_INVALID_DATA;
	}

	if (out == nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	out = elf_header;
	return ERROR_SUCCESS;
}

error_code elf_executable::get_program_headers(OUT Elf64_Phdr* out, OUT size_t* count)
{
	if (!m_valid)
	{
		return -ERROR_INVALID_DATA;
	}

	if (out == nullptr || count == nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	*out = *prog_headers;
	*count = elf_header->e_phnum;

	return ERROR_SUCCESS;
}

error_code elf_executable::get_section_headers(OUT Elf64_Shdr* out, OUT size_t* count)
{
	if (!m_valid)
	{
		return -ERROR_INVALID_DATA;
	}

	if (out == nullptr || count == nullptr)
	{
		return -ERROR_INVALID_ARG;
	}

	*out = *section_headers;
	*count = elf_header->e_shnum;

	return ERROR_SUCCESS;
}