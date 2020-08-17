#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"

using namespace pci::express;
using namespace libkernel;

struct dev_list
{
	list_head dev_head;
	size_t count;

	void add(pcie_device* dev)
	{
		list_add(&dev->list, &dev_head);
		count++;
	}

	dev_list() : dev_head(list_head{}),
				 count(0)
	{
		list_init(&this->dev_head);
	}
};

enum DEV_SUPPORT
{
	DS_UNSUPPORTED, DS_SUPPORTED
};

dev_list supported{};
dev_list unsupported{};
size_t dev_total = 0;

static inline DEV_SUPPORT pcie_device_init(pcie_device* dev)
{
	DEV_SUPPORT ret = DS_UNSUPPORTED;
	auto cmd_stat_reg_val = dev->read_dword(PCIE_HEADER_OFFSET_STATUS_COMMAND);
	auto cmd_stat_reg = *((pci_command_status_reg*)(&cmd_stat_reg_val));

	if (cmd_stat_reg.status.capabilities_list == 0x1)
	{
		ret = DS_SUPPORTED;


	}
	else
	{
		ret = DS_UNSUPPORTED;
	}

	return ret;
}

static inline void pcie_enumerate_device(uintptr_t base_address,
	uint16_t seg,
	uint8_t start_bus,
	uint8_t bus,
	uint8_t dev_no)
{

	for (uint8_t func = 0; func < 8; func++)
	{
		uint32_t d = ((uint32_t)(bus - start_bus) << 20u) | ((uint32_t)dev_no << 15u) | ((uint32_t)func << 12u);
		uint8_t* config = (uint8_t*)P2V(base_address + d);
		uint32_t id = *(uint32_t*)config;

		if ((id & 0xFFFFu) == 0xFFFF)
		{
			continue;
		}

		pcie_device* dev = new pcie_device;

		KDEBUG_ASSERT(dev != nullptr);

		dev->bus = bus;
		dev->dev = dev_no;
		dev->func = func;
		dev->segment_group = seg;
		dev->config = config;

		auto sup = pcie_device_init(dev);

		if (sup == DS_SUPPORTED)
		{
			supported.add(dev);
		}
		else if (sup == DS_UNSUPPORTED)
		{
			unsupported.add(dev);
		}
	}
}

static inline void pcie_enumerate_entry(acpi::mcfg_entry entry)
{
	for (uint16_t bus = entry.start_pci_bus; bus < entry.end_pci_bus; bus++)
	{
		for (uint8_t dev = 0; dev < 32; dev++)
		{
			uint8_t* config =
				(uint8_t*)P2V(entry.base_address + ((((uint8_t)(bus - entry.start_pci_bus)) << 20u) | (dev << 15u)));

			if (((*(uint32_t*)config) & 0xFFFFu) != 0xFFFFu)
			{
				uint32_t header = *(uint32_t*)(config + 0x0C);
				header >>= 16u;
				header &= 0x7Fu;

				if (header == 0x00)
				{
					pcie_enumerate_device(entry.base_address, entry.pci_segment_group, entry.start_pci_bus, bus, dev);
				}
				else
				{
					kdebug::kdebug_warning("Skipping unsupported header type: %02x\n", header);
				}

			}
		}
	}
}

uint32_t pcie_device::read_dword(size_t off)
{
	return (*(uint32_t*)(this->config + (off)));
}

void pcie_device::write_dword(size_t off, uint32_t value)
{
	(*(uint32_t*)(this->config + (off))) = (value);
}

error_code pci::express::pcie_init(acpi::acpi_mcfg* mcfg)
{

	size_t entry_count =
		(mcfg->header.length - sizeof(acpi::acpi_desc_header) - sizeof(mcfg->reserved)) / sizeof(acpi::mcfg_entry);

	for (size_t i = 0; i < entry_count; i++)
	{
		pcie_enumerate_entry(mcfg->config_spaces[i]);
	}

	dev_total = supported.count + unsupported.count;

	return ERROR_SUCCESS;
}