#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"

using namespace pci::express;
using namespace libkernel;

list_head supported_dev_head{};
list_head unsupported_dev_head{};

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

		if ((id & 0xFFFFu) != 0xFFFF)
		{
			pcie_device* dev = new pcie_device;

			KDEBUG_ASSERT(dev != nullptr);

			dev->bus = bus;
			dev->dev = dev_no;
			dev->func = func;
			dev->segment_group = seg;
			dev->config = config;

			list_add(&dev->list, &supported_dev_head);
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

error_code pci::express::pcie_init(acpi::acpi_mcfg* mcfg)
{
	list_init(&supported_dev_head);
	list_init(&unsupported_dev_head);

	size_t entry_count =
		(mcfg->header.length - sizeof(acpi::acpi_desc_header) - sizeof(mcfg->reserved)) / sizeof(acpi::mcfg_entry);

	for (size_t i = 0; i < entry_count; i++)
	{
		pcie_enumerate_entry(mcfg->config_spaces[i]);
	}

	return ERROR_SUCCESS;
}