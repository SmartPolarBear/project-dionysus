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

#include "fs/device/ATABlockDevice.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ahci;


error_code file_system::ATABlockDevice::ioctl([[maybe_unused]]size_t req, [[maybe_unused]]void* args)
{
	return -ERROR_UNSUPPORTED;
}

size_t file_system::ATABlockDevice::write(const void* buf, uintptr_t offset, size_t sz)
{
	if (offset % this->block_size)
	{
		return -ERROR_INVALID;
	}

	if (sz % this->block_size)
	{
		return -ERROR_INVALID;
	}

	ahci_port* port = reinterpret_cast<ahci_port*>(this->dev_data);

	if (port == nullptr)
	{
		return -ERROR_INVALID;
	}

	logical_block_address lba = offset / this->block_size;

	auto ret = ahci_port_send_command(port, ATA_CMD_WRITE_DMA_EX, false, lba, const_cast<void*>(buf), sz);

	return ret;
}

size_t file_system::ATABlockDevice::read(void* buf, uintptr_t offset, size_t sz)
{
	if (offset % this->block_size)
	{
		return -ERROR_INVALID;
	}

	if (sz % this->block_size)
	{
		return -ERROR_INVALID;
	}

	ahci_port* port = reinterpret_cast<ahci_port*>(this->dev_data);

	if (port == nullptr)
	{
		return -ERROR_INVALID;
	}

	logical_block_address lba = offset / this->block_size;

	auto ret = ahci_port_send_command(port, ATA_CMD_READ_DMA_EX, false, lba, buf, sz);

	return ret;
}

file_system::ATABlockDevice::~ATABlockDevice() = default;

file_system::ATABlockDevice::ATABlockDevice(ahci::ahci_port* port)
{
	this->dev_data = port;
	this->flags = 0;
	this->block_size = ATA_DEFAULT_SECTOR_SIZE;
}

error_code file_system::ATABlockDevice::mmap([[maybe_unused]]uintptr_t base,
	[[maybe_unused]]size_t page_count,
	[[maybe_unused]]int prot,
	[[maybe_unused]]size_t flags)
{
	return -ERROR_UNSUPPORTED;
}
