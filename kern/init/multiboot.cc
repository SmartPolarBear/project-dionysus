#include "sys/multiboot.h"
#include "arch/amd64/x86.h"
#include "sys/bootmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/vm.h"

#include "lib/libc/string.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

using namespace multiboot;

// the boot header defined by Multiboot2 , aligned 8 bytes
struct alignas(8) multiboot2_boot_info
{
    multiboot_uint32_t total_size;
    multiboot_uint32_t reserved;
    multiboot_tag tags[0];
};

// the *PHYSICAL* address for multiboot2_boot_info and the magic number
extern "C" void *mbi_structptr = nullptr;
extern "C" uint32_t mbi_magicnum = 0;

// the pointer storing the *VIRTUAL* address for multiboot2_boot_info and the magic number
multiboot2_boot_info *mboot_info = nullptr;
multiboot_tag_ptr multiboot_tags[TAGS_COUNT_MAX];

//Initialize the mboot_info and check the magic number
//May panic the kernel if magic number checking failes
void multiboot::init_mbi(void)
{
    if (mbi_magicnum != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        KDEBUG_GENERALPANIC("Can't verify the magic number.\n");
    }

    auto primitive = P2V<decltype(mboot_info)>(mbi_structptr);
    KDEBUG_ASSERT(primitive->total_size < vm::BOOTMM_BLOCKSIZE);

    mboot_info = reinterpret_cast<decltype(mboot_info)>(vm::bootmm_alloc());
    memset(mboot_info, 0, vm::BOOTMM_BLOCKSIZE);
    memmove(mboot_info, mbi_structptr, primitive->total_size);

    for (auto ptr : multiboot_tags)
    {
        ptr = nullptr;
    }

    //FIXME: add a barrier to prevent something crack the mboot info
    // I guess the crack is because an imperfect memory layout.
    [[maybe_unused]] char *p = vm::bootmm_alloc();

    // console::printf("mbootinfo=0x%p,p=0x%p\n", mboot_info, p);
    parse_multiboot_tags();
}

//Map the tag type to the corresponding pointer
void multiboot::parse_multiboot_tags(void)
{
    for (multiboot_tag *tag = mboot_info->tags;
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        multiboot_tags[tag->type] = tag;
    }
}

multiboot_tag_const_readonly_ptr multiboot::aquire_tag_ptr(size_t type)
{
    return multiboot_tags[type];
}