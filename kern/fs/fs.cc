#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

using namespace libkernel;

list_head fs_class_head;
list_head fs_mount_head;

PANIC void file_system::fs_init()
{
	// Initialize list heads
	list_init(&fs_class_head);
	list_init(&fs_mount_head);

	// register file system classes
	g_ext2fs.register_this();

	// Initialize devfs root to load real hardware
	auto ret = file_system::init_devfs_root();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	// Initialize the fs hardware
	ret = ahci::ahci_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

}

error_code file_system::fs_create(fs_class_base* fs_class, device_class* dev, size_t flags, void* data)
{
	KDEBUG_NOT_IMPLEMENTED;
	return ERROR_SUCCESS;
}

error_code file_system::fs_register(fs_class_base* fs_class)
{
	if (fs_find(fs_class->get_id()) != nullptr)
	{
		return -ERROR_REWRITE;
	}

	list_add(&fs_class->link, &fs_class_head);
	return ERROR_SUCCESS;
}

file_system::fs_class_base* file_system::fs_find(fs_class_id id)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		// FIXME
		auto entry = list_entry(iter, fs_class_base, link);
		if (entry->get_id() == id)
		{
			return entry;
		}
	}
	return nullptr;
}

file_system::fs_class_base* file_system::fs_find(const char* name)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		// FIXME
		auto entry = list_entry(iter, fs_class_base, link);
		if (strcmp(entry->get_name(), name) == 0)
		{
			return entry;
		}
	}
	return nullptr;
}

