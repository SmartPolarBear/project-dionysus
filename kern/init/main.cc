/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-13 13:27:00
 * @ Description: the entry point for kernel in C++
 */

#include "arch/amd64/x86.h"
#include "boot/multiboot2.h"
#include "drivers/console/cga.h"
#include "drivers/console/console.h"
#include "lib/libc/string.h"
#include "lib/libcxx/new.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/vm.h"

using multiboot::aquire_tag;
using multiboot::init_mbi;
using multiboot::parse_multiboot_tags;

extern char end[];

extern "C" [[noreturn]] void kmain() {
    // process the multiboot information
    init_mbi();
    parse_multiboot_tags();
    
    const multiboot_tag_mmap *memtag = reinterpret_cast<multiboot_tag_mmap*>(aquire_tag(MULTIBOOT_TAG_TYPE_MMAP));
    if (memtag == nullptr)
    {
        //TODO: panic;
        console::printf("Can't find multiboot_tag_mmap.\n");
    }

    size_t entry_cnt = (memtag->size - memtag->entry_size - sizeof(*memtag)) / memtag->entry_size;
    for (size_t i = 0ul; i < entry_cnt; i++)
    {
        console::printf("addr=0x%x (%d), len=%x (%d), type=%d\n",
                        memtag->entries[i].addr, memtag->entries[i].addr, memtag->entries[i].len,
                        memtag->entries[i].len, memtag->entries[i].type);
    }

    vm::bootmm_init(end, (void *)P2V(4 * 1024 * 1024));
    // vm::kvm_init(entry_cnt, memtag->entries);

    // char *c = _kernel_virtual_end + 0x100000 + 0x100000;
    // *c = 0x12345;

    console::printf("Hello world! build=%d\n", 3);

    for (;;)
        ;
}

//C++ ctors and dtors
extern "C"
{
    using constructor_type = void (*)();
    extern constructor_type start_ctors;
    extern constructor_type end_ctors;

    void call_ctors(void)
    {
        for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
        {
            (*ctor)();
        }
    }

    void call_dtors(void)
    {
        //TODO : call global desturctors
    }
}
