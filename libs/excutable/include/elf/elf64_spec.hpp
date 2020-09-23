#pragma once
#include "system/types.h"

namespace executable
{
	using Elf64_Addr = uint64_t;
	static_assert(sizeof(Elf64_Addr) == 8);

	using Elf64_Off = uint64_t;
	static_assert(sizeof(Elf64_Off) == 8);

	using Elf64_Half = uint16_t;
	static_assert(sizeof(Elf64_Half) == 2);

	using Elf64_Word = uint32_t;
	static_assert(sizeof(Elf64_Word) == 4);

	using Elf64_Sword = int32_t;
	static_assert(sizeof(Elf64_Sword) == 4);

	using Elf64_Xword = uint64_t;
	static_assert(sizeof(Elf64_Xword) == 8);

	using Elf64_Sxword = int64_t;
	static_assert(sizeof(Elf64_Sxword) == 8);

	struct Elf64_Ehdr
	{
		unsigned char e_ident[16]; /* ELF identification */
		Elf64_Half e_type; /* Object file type */
		Elf64_Half e_machine; /* Machine type */
		Elf64_Word e_version; /* Object file version */
		Elf64_Addr e_entry; /* Entry point address */
		Elf64_Off e_phoff; /* Program header offset */
		Elf64_Off e_shoff; /* Section header offset */
		Elf64_Word e_flags; /* Processor-specific flags */
		Elf64_Half e_ehsize; /* ELF header size */
		Elf64_Half e_phentsize; /* Size of program header entry */
		Elf64_Half e_phnum; /* Number of program header entries */
		Elf64_Half e_shentsize; /* Size of section header entry */
		Elf64_Half e_shnum; /* Number of section header entries */
		Elf64_Half e_shstrndx; /* Section name string table index */
	}__attribute__((__packed__));

	struct Elf64_Shdr
	{
		Elf64_Word sh_name; /* Section name */
		Elf64_Word sh_type; /* Section type */
		Elf64_Xword sh_flags; /* Section attributes */
		Elf64_Addr sh_addr; /* Virtual address in memory */
		Elf64_Off sh_offset; /* Offset in file */
		Elf64_Xword sh_size; /* Size of section */
		Elf64_Word sh_link; /* Link to other section */
		Elf64_Word sh_info; /* Miscellaneous information */
		Elf64_Xword sh_addralign; /* Address alignment boundary */
		Elf64_Xword sh_entsize; /* Size of entries, if section has table */
	}__attribute__((__packed__));

	struct Elf64_Sym
	{
		Elf64_Word st_name; /* Symbol name */
		unsigned char st_info; /* Type and Binding attributes */
		unsigned char st_other; /* Reserved */
		Elf64_Half st_shndx; /* Section table index */
		Elf64_Addr st_value; /* Symbol value */
		Elf64_Xword st_size; /* Size of object (e.g., common) */
	}__attribute__((__packed__));

	struct Elf64_Rel
	{
		Elf64_Addr r_offset; /* Address of reference */
		Elf64_Xword r_info; /* Symbol index and type of relocation */
	} __attribute__((__packed__));

	struct Elf64_Rela
	{
		Elf64_Addr r_offset; /* Address of reference */
		Elf64_Xword r_info; /* Symbol index and type of relocation */
		Elf64_Sxword r_addend; /* Constant part of expression */
	}__attribute__((__packed__));

	struct Elf64_Phdr
	{
		Elf64_Word p_type; /* Type of segment */
		Elf64_Word p_flags; /* Segment attributes */
		Elf64_Off p_offset; /* Offset in file */
		Elf64_Addr p_vaddr; /* Virtual address in memory */
		Elf64_Addr p_paddr; /* Reserved */
		Elf64_Xword p_filesz; /* Size of segment in file */
		Elf64_Xword p_memsz; /* Size of segment in memory */
		Elf64_Xword p_align; /* Alignment of segment */
	}__attribute__((__packed__));

	enum Elf_Ident
	{
		EI_MAG0 = 0, // 0x7F
		EI_MAG1 = 1, // 'E'
		EI_MAG2 = 2, // 'L'
		EI_MAG3 = 3, // 'F'
		EI_CLASS = 4, // Architecture (32/64)
		EI_DATA = 5, // Byte Order
		EI_VERSION = 6, // ELF Version
		EI_OSABI = 7, // OS Specific
		EI_ABIVERSION = 8, // OS Specific
		EI_PAD = 9,  // Padding
		EI_NIDENT = 16 // Size of e_ident[]
	};

	enum Elf_Obj_Class
	{
		ELFCLASS32 = 1,
		ELFCLASS64 = 2,
	};

	enum Elf_Data_Encodings
	{
		ELFDATA2LSB = 1,
		ELFDATA2MSB = 2
	};

	enum Elf_OSABI
	{
		ELFOSABISYSV = 0,
		ELFOSABIHPUX = 1,
		ELFOSABISTANDALONE = 255,
	};

	enum Elf_Object_File_Types
	{
		ET_NONE = 0, //No file type
		ET_REL = 1,//Relocatable object file
		ET_EXEC = 2,//Executable file
		ET_DYN = 3,//Shared object file
		ET_CORE = 4,//Core file
		ET_LOOS = 0xFE00,//Environment-specific use
		ET_HIOS = 0xFEFF,
		ET_LOPROC = 0xFF00,// Processor-specific use
		ET_HIPROC = 0xFFFF
	};

	enum Elf_Special_Section_Indexes
	{
		SHN_UNDEF = 0,//Used to mark an undefined or meaningless section reference
		SHN_LOPROC = 0xFF00,// Processor-specific use
		SHN_HIPROC = 0xFF1F,
		SHN_LOOS = 0xFF20,// Environment-specific use
		SHN_HIOS = 0xFF3F,
		SHN_ABS = 0xFFF1,// Indicates that the corresponding reference is an absolute value
		SHN_COMMON = 0xFFF2, // Indicates a symbol that has been
		// declared as a common superblock
		// (Fortran COMMON or C tentative declaration)
	};

	enum Elf_Section_Types
	{
		SHT_NULL = 0,// Marks an unused section header
		SHT_PROGBITS = 1,// Contains information defined by the program
		SHT_SYMTAB = 2,// Contains a linker symbol table
		SHT_STRTAB = 3,// Contains a string table
		SHT_RELA = 4,// Contains “Rela” type relocation entries
		SHT_HASH = 5,// Contains a symbol hash table
		SHT_DYNAMIC = 6,// Contains dynamic linking tables
		SHT_NOTE = 7,// Contains note information
		SHT_NOBITS = 8,// Contains uninitialized space; does not occupy any space in the file
		SHT_REL = 9,// Contains “Rel” type relocation entries
		SHT_SHLIB = 10,// Reserved
		SHT_DYNSYM = 11, //Contains a dynamic loader symbol table
		SHT_LOOS = 0x60000000,// Environment-specific use
		SHT_HIOS = 0x6FFFFFFF,
		SHT_LOPROC = 0x70000000,// Processor-specific use
		SHT_HIPROC = 0x7FFFFFFF
	};

	enum Elf_Section_Attributes
	{
		SHF_WRITE = 0x1, // Section contains writable data
		SHF_ALLOC = 0x2, // Section is allocated in memory image of program
		SHF_EXECINSTR = 0x4,// Section contains executable instructions
		SHF_MASKOS = 0x0F000000, //Environment-specific use
		SHF_MASKPROC = 0xF0000000,//Processor-specific use
	};

	enum Elf_Program_Header_Types
	{
		PT_NULL = 0,// Unused entry
		PT_LOAD = 1,// Loadable segment
		PT_DYNAMIC = 2,// Dynamic linking tables
		PT_INTERP = 3,// Program interpreter path name
		PT_NOTE = 4 //Note sections
	};

	enum Elf_Pogram_Header_Attributes
	{
		PF_X = 0x1,//Execute permission
		PF_W = 0x2,//Write permission
		PF_R = 0x4,//Read permission
		PF_MASKOS = 0x00FF0000,//These flag bits are reserved for environment-specific use
		PF_MASKPROC = 0xFF000000,//These flag bits are reserved for processor-specific use
	};
}