#include "arch/amd64/x86.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "lib/libc/string.h"
#include "lib/libkern/data/list.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

using namespace multiboot;

using libk::list_add;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

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

extern uint8_t end[]; // kernel.ld

// the pointer storing the *VIRTUAL* address for multiboot2_boot_info and the magic number
multiboot2_boot_info *mboot_info = nullptr;

// multiboot_tag_ptr multiboot_tags[TAGS_COUNT_MAX];

struct multiboot_tag_node
{
    multiboot_tag_ptr tag;
    list_head node;
};

list_head multiboot_tag_heads[TAGS_COUNT_MAX] = {};

multiboot_tag_node allnodes[TAGS_COUNT_MAX * 4] = {};
size_t allnodes_counter = 0;
static inline multiboot_tag_node *alloc_new_node(void)
{
    KDEBUG_ASSERT(allnodes_counter < (sizeof(allnodes) / sizeof(allnodes[0])));
    return allnodes + (allnodes_counter++);
}

//Initialize the mboot_info and check the magic number
//May panic the kernel if magic number checking failes
void multiboot::init_mbi(void)
{
    if (mbi_magicnum != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        KDEBUG_RICHPANIC("Can't verify the magic number.\n",
                         "KERNEL PANIC: MULTIBOOT",
                         false,
                         "The multiboot magic number given is %d\n", mbi_magicnum);
    }

    auto primitive = P2V<decltype(mboot_info)>(mbi_structptr);
    KDEBUG_ASSERT(primitive->total_size < PHYSICAL_PAGE_SIZE);

    mboot_info = reinterpret_cast<decltype(mboot_info)>(end + PHYSICAL_PAGE_SIZE);
    memset(mboot_info, 0, PHYSICAL_PAGE_SIZE);
    memmove(mboot_info, mbi_structptr, primitive->total_size);

    for (size_t i = 0; i < TAGS_COUNT_MAX; i++)
    {
        list_init(&multiboot_tag_heads[i]);
    }


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
        list_head *head = &multiboot_tag_heads[tag->type];

        auto newnode = alloc_new_node();
        newnode->tag = tag;

        list_add(&newnode->node, head);
    }
}

multiboot_tag_const_readonly_ptr multiboot::acquire_tag_ptr(size_t type)
{
    multiboot_tag_ptr buf[1] = {};
    [[maybe_unused]] auto cnt = get_all_tags(type, buf, 1);
    return buf[0];
}

multiboot_tag_const_readonly_ptr multiboot::acquire_tag_ptr(size_t type, aquire_tag_ptr_predicate pred)
{
    multiboot_tag_ptr buf[TAGS_COUNT_MAX] = {};
    size_t count = get_all_tags(type, nullptr, 0);
    if (!count)
    {
        return nullptr;
    }

    count = get_all_tags(type, buf, count);

    for (size_t i = 0; i < count; i++)
    {
        if (pred(buf[i]))
        {
            return buf[i];
        }
    }

    return nullptr;
}

size_t multiboot::get_all_tags(size_t type, multiboot_tag_ptr *buf, size_t bufsz)
{
    list_head *iter = nullptr, *head = &multiboot_tag_heads[type];
    size_t count = 0;

    list_for(iter, head)
    {
        if (buf != nullptr)
        {
            if (count >= bufsz)
            {
                break;
            }
            auto entry = list_entry(iter, multiboot_tag_node, node);
            buf[count++] = entry->tag;
        }
        else
        {
            count++;
        }
    }

    return count;
}