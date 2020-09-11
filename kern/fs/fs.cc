#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

PANIC void file_system::fs_init()
{

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
	return ERROR_SUCCESS;
}