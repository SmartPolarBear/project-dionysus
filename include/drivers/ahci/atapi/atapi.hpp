#pragma once

#include "system/types.h"
#include "drivers/pci/pci_device.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/identify_device.hpp"

namespace ahci
{
	error_code atapi_identify_device(ahci_port* port);
}