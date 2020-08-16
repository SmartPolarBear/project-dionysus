#include "arch/amd64/port_io.h"

#include "system/types.h"

#include "drivers/pci/pci.hpp"

using namespace pci::legacy;

uint32_t pci::legacy::pci_config_read_dword_legacy(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	auto addr = (((uint32_t)bus) << 16u) |
		(((uint32_t)slot) << 11u) |
		(((uint32_t)func) << 8u) |
		(offset & ~0x3u) |
		(1u << 31u);

	outl(PCI_CONFIG_ADDRESS, addr);
	auto ret = inl(PCI_CONFIG_DATA);

	return ret;
}