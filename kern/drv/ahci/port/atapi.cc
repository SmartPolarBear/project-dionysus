#include "common.hpp"
#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"
#include "drivers/ahci/atapi/atapi.hpp"

#include "../../../libs/basic_io/include/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

error_code ahci::atapi_port_identify_device(ahci_port* port)
{
	return common_identify_device(port, true);
}

error_code ahci::atapi_port_read(ahci_port* port, logical_block_address lba, void* buf, size_t sz)
{
	auto ret = ahci_port_send_command(port, ATA_CMD_PACKET, true, lba, buf, sz);
	return ret;
}

// ATAPI device, usually CD-ROM, can't be written.