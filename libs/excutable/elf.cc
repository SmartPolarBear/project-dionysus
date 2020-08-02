#include "elf/elf.hpp"
#include "system/error.h"

#include <cstring>

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
		return -ERROR_INVALID;
	}

	m_valid = true;

	prog_headers = (Elf64_Phdr*)(bin.data + elf_header->e_phoff);
	section_headers = (Elf64_Shdr*)(bin.data + elf_header->e_shoff);

	return ERROR_SUCCESS;
}

uint8_t* elf_executable::get_data() const
{
	return bin.data;
}

error_code elf_executable::get_elf_header(OUT Elf64_Ehdr** out) const
{
	if (!m_valid)
	{
		return -ERROR_INVALID;
	}

	if (*out == nullptr)
	{
		*out = elf_header;
	}
	else
	{
		**out = *elf_header;
	}

	return ERROR_SUCCESS;
}

error_code elf_executable::get_program_headers(OUT Elf64_Phdr** out, OUT size_t* count) const
{
	if (!m_valid)
	{
		return -ERROR_INVALID;
	}

	if (count == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (*out == nullptr)
	{
		*out = prog_headers;
	}
	else
	{
		memmove(*out, prog_headers, elf_header->e_phnum);
	}

	*count = elf_header->e_phnum;

	return ERROR_SUCCESS;
}

error_code elf_executable::get_section_headers(OUT Elf64_Shdr** out, OUT size_t* count) const
{
	if (!m_valid)
	{
		return -ERROR_INVALID;
	}

	if (count == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (*out == nullptr)
	{
		*out = section_headers;
	}
	else
	{
		memmove(*out, section_headers, elf_header->e_shnum);
	}

	*count = elf_header->e_shnum;

	return ERROR_SUCCESS;
}