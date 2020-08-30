#pragma once

#include "system/types.h"
#include "drivers/pci/pci_device.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/identify_device.hpp"

namespace ahci
{
	error_code ata_port_identify_device(ahci_port* port);

	error_code ata_port_read(ahci_port* port, logical_block_address lba, void* buf, size_t sz);

	error_code ata_port_write(ahci_port* port, logical_block_address lba, const void* buf, size_t sz);

}