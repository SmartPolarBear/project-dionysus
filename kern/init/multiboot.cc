#include "sys/multiboot.h"
#include "arch/amd64/x86.h"
#include "drivers/console/console.h"
#include "sys/memlayout.h"

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
    mboot_info = P2V<multiboot2_boot_info>(mbi_structptr);
    if (mbi_magicnum != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        //TODO : panic
        console::printf("Can't verify the magic number.\n");
    }

    for (auto ptr : multiboot_tags)
    {
        ptr = nullptr;
    }
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

const multiboot_tag_ptr multiboot::aquire_tag(size_t type)
{
    return multiboot_tags[type];
}