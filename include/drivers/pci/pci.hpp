#pragma once
#include "system/types.h"

namespace pci
{
	constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
	constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

	// legacy mechanism of configuration space access
	uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
}