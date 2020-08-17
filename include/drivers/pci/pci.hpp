#pragma once
#include "system/types.h"

#include "drivers/acpi/acpi.h"
#include "drivers/pci/pci_header_common.hpp"
#include "drivers/pci/pci_header_t0.hpp"
#include "drivers/pci/pci_header_t1.hpp"
#include "drivers/pci/pci_header_t2.hpp"

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

		struct pcie_device
		{
			uint8_t bus, dev, func;

			uint16_t segment_group;
			uint8_t* config;

			list_head list;

			uint32_t read_dword(size_t off);
			void write_dword(size_t off, uint32_t value);
		};

		error_code pcie_init(acpi::acpi_mcfg* mcfg);
	}

	// initialize PCI and PCIe
	void pci_init();
}