/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-12 22:22:10
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
#include "sys/param.h"
#include "sys/vm.h"

// the boot header defined by Multiboot2 , aligned 8 bytes
struct alignas(8) multiboot2_boot_info
{
    multiboot_uint32_t total_size;
    multiboot_uint32_t reserved;
    multiboot_tag tags[0];
};

// the *PHYSICAL* address for multiboot2_boot_info and the magic number
extern "C" void *mbi_structptr = nullptr;
extern "C" void *mbi_magicptr = nullptr;
// the pointer storing the *VIRTUAL* address for multiboot2_boot_info
multiboot2_boot_info *mboot_info = nullptr;
// the magic number
uint32_t magic_number = 0;

// multiboot_tags maps TAG_TYPE -> the pointer to the tag
namespace multiboot
{
using multiboot_tag_ptr = multiboot_tag *;
constexpr size_t TAGS_COUNT_MAX = 24;
multiboot_tag_ptr multiboot_tags[TAGS_COUNT_MAX];

static inline void parse_multiboot_tags()
{
    for (multiboot_tag *tag = mboot_info->tags;
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        multiboot_tags[tag->type] = tag;
    }
}
} // namespace multiboot

// extern char _kernel_virtual_start[];  //kernel.ld
// extern char _kernel_virtual_end[];    //kernel.ld
// extern char data[];                   //kernel.ld
// extern char edata[];                  //kernel.ld
// extern char _kernel_physical_start[]; //kernel.ld
// extern char _kernel_physical_end[];   //kernel.ld
extern char end[];

extern "C" [[noreturn]] void kmain() {
    int size = 123;
    console::printf("&size=0x%p\n", &size);
    mboot_info = new (P2V((mbi_structptr)));
    console::printf("mboot_info=0x%p,size=%d\n", mboot_info, mboot_info->total_size);

    // mboot_info = P2V<multiboot2_boot_info>(mboot_addr);
    // parse_multiboot_tags();
    // multiboot_tag_mmap *mmap = reinterpret_cast<multiboot_tag_mmap *>(multiboot_tags[MULTIBOOT_TAG_TYPE_MMAP]);
    // size_t entry_cnt = (mmap->size - mmap->entry_size - sizeof(*mmap)) / mmap->entry_size;

    // vm::bootmm_init(end, end + 0x100000);
    // vm::kvm_init(entry_cnt, mmap->entries);

    // char *c = _kernel_virtual_end + 0x100000 + 0x100000;
    // *c = 0x12345;

    console::printf("Hello world! %d\n", 122);
    // console::printf("pkstart=0x%x\npkend=0x%x\n",
    //                 _kernel_physical_start,
    //                 _kernel_physical_end);
    // console::printf("vkstart=0x%x\nvdata=0x%x\nvedata=0x%x\nvkend=0x%x\n",
    //                 _kernel_virtual_start,
    //                 data,
    //                 edata,
    //                 _kernel_virtual_end);

    for (;;)
        ;
}

//C++ ctors
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
}
