#pragma once
#include "system/types.h"
#include "system/concepts.hpp"

#include "drivers/acpi/acpi.h"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/apic/traps.h"

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

		error_code pcie_device_config_msi(IN pci_device* dev, trap::trap_handle int_handle);
	}

	// initialize PCI and PCIe
	void pci_init();
}