#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

error_code ahci::ahci_init()
{


	return ERROR_SUCCESS;
}