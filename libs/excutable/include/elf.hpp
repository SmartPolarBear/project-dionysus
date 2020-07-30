#pragma once

#include "system/types.h"
#include "elf64_spec.hpp"

namespace executable
{

	struct binary
	{
		size_t size;
		void* data;
	};

	class elf_executable
	{
	 private:
		binary bin;

		bool m_valid;

		Elf64_Ehdr* elf_header;
		Elf64_Phdr* prog_headers;
		Elf64_Shdr* section_headers;

	 public:
		elf_executable()
			: m_valid(true),
			  elf_header(nullptr), prog_headers(nullptr), section_headers(nullptr)
		{
		}

		error_code parse(binary bin);

		error_code get_elf_header(OUT Elf64_Ehdr* out);

		error_code get_program_headers(OUT Elf64_Phdr* out, OUT size_t *count);

		error_code get_section_headers(OUT Elf64_Shdr* out, OUT size_t *count);
	};
}