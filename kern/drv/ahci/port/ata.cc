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

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

error_code ahci::ata_port_identify_device(ahci_port* port)
{
	return common_identify_device(port, false);
}

error_code ahci::ata_port_read(ahci_port* port, logical_block_address lba, void* buf, size_t sz)
{
	auto ret = ahci_port_send_command(port, ATA_CMD_READ_DMA_EX, false, lba, buf, sz);
	return ret;
}

error_code ahci::ata_port_write(ahci_port* port, logical_block_address lba, const void* buf, size_t sz)
{
	auto ret = ahci_port_send_command(port, ATA_CMD_WRITE_DMA_EX, false, lba, const_cast<void*>(buf), sz);
	return ret;
}

