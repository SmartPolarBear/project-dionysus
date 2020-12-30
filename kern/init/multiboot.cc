#include "arch/amd64/cpu/x86.h"

#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"

#include "string.h"
#include "kbl/data/pod_list.h"

#include "drivers/console/console.h"
#include "debug/kdebug.h"

using namespace multiboot;

using kbl::list_add;
using kbl::list_for_each;
using kbl::list_init;
using kbl::list_remove;

// define the multiboot parameters
extern "C" void* mbi_structptr = nullptr;
extern "C" uint32_t mbi_magicnum = 0;

// the boot header defined by Multiboot2 , aligned 8 bytes
// the pointer storing the *VIRTUAL* address for multiboot2_boot_info and the magic number
struct alignas(8) multiboot2_boot_info
{
	multiboot_uint32_t total_size;
	multiboot_uint32_t reserved;
	multiboot_tag tags[0];
} * mboot_info = nullptr;

struct multiboot_tag_node
{
	multiboot_tag_ptr tag;
	list_head node;
};

list_head multiboot_tag_heads[TAGS_COUNT_MAX] = {};

multiboot_tag_node allnodes[TAGS_COUNT_MAX * 4] = {};
size_t allnodes_counter = 0;

static inline multiboot_tag_node* alloc_new_node(void)
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

	// this is the original mboot info
	auto primitive = P2V<decltype(mboot_info)>(mbi_structptr);
	KDEBUG_ASSERT(primitive->total_size < PAGE_SIZE);

	// move it away to avoid being cracked
	mboot_info = reinterpret_cast<decltype(mboot_info)>(end + PAGE_SIZE);
	memset(mboot_info, 0, PAGE_SIZE);
	memmove(mboot_info, mbi_structptr, primitive->total_size);

	for (size_t i = 0; i < TAGS_COUNT_MAX; i++)
	{
		list_init(&multiboot_tag_heads[i]);
	}

	//Map the tag type to the corresponding pointer
	for (multiboot_tag* tag = mboot_info->tags;
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (multiboot_tag*)((multiboot_uint8_t*)tag + ((tag->size + 7) & ~7)))
	{
		list_head* head = &multiboot_tag_heads[tag->type];

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

size_t multiboot::get_all_tags(size_t type, multiboot_tag_ptr* buf, size_t bufsz)
{
	list_head* iter = nullptr, * head = &multiboot_tag_heads[type];
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

error_code multiboot::find_module_by_cmdline(IN const char* cmdline, OUT size_t* sz, OUT uint8_t** mod_bin)
{
	multiboot_tag_module *tag = nullptr;

	// search for the tag;

	multiboot_tag_ptr buf[TAGS_COUNT_MAX] = {};
	size_t count = get_all_tags(MULTIBOOT_TAG_TYPE_MODULE, nullptr, 0);
	if (!count)
	{
		return -ERROR_INVALID;
	}

	count = get_all_tags(MULTIBOOT_TAG_TYPE_MODULE, buf, count);

	for (size_t i = 0; i < count; i++)
	{
		multiboot_tag_module* mdl_tag = reinterpret_cast<decltype(mdl_tag)>(buf[i]);

		if (strncmp(mdl_tag->cmdline, cmdline, strlen(cmdline)) == 0)
		{
			tag = mdl_tag;
			break;
		}
	}

	if (tag == nullptr)
	{
		return -ERROR_INVALID;
	}

	// fill result variables
	*sz = tag->mod_end - tag->mod_start + 1;
	*mod_bin = (uint8_t*)P2V(tag->mod_start);

	return ERROR_SUCCESS;
}
