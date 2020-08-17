#pragma once
#include "system/types.h"

#include "drivers/acpi/acpi.h"

#include "data/List.h"

namespace pci
{
	namespace legacy
	{
		constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
		constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

		// legacy mechanism of configuration space access
		[[deprecated("Legacy PCI is not planned to be supported."), maybe_unused]]
		uint32_t pci_config_read_dword_legacy(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
	}

	namespace express
	{
		error_code pcie_init(acpi::acpi_mcfg* mcfg);

		struct pcie_device
		{
			uint8_t bus, dev, func;

			uint16_t segment_group;
			uint8_t* config;

			list_head list;
		};
	}

	// initialize PCI and PCIe
	void pci_init();
}