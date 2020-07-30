#pragma once

#include "system/process.h"
#include "system/types.h"

constexpr size_t ELF_MAGIC = 0x464C457FU;

struct elfhdr
{
    uint32_t magic; // must equal ELF_MAGIC
    uint8_t elf[12];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uintptr_t entry;
    uintptr_t phoff;
    uintptr_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
}__attribute__((__packed__));

struct proghdr
{
    uint32_t type;
    uint32_t flags;
    uint64_t off;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
}__attribute__((__packed__));

// Values for Proghdr type
enum prog_header_types
{
    ELF_PROG_LOAD = 1
};

enum prog_header_flags
{
    ELF_PROG_FLAG_EXEC = 1,
    ELF_PROG_FLAG_WRITE = 2,
    ELF_PROG_FLAG_READ = 4,
};

error_code elf_load_binary(IN process::process_dispatcher *proc,
                           IN uint8_t *bin,
                           OUT uintptr_t *entry_addr);