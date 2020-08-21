#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "fs/fs.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

PANIC void file_system::fs_init()
{
	// firstly, initialize the fs hardware
	auto ret = ahci::ahci_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}


}

