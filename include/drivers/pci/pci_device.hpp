#pragma once

#include "system/types.h"
#include "system/concepts.hpp"

#include "drivers/acpi/acpi.h"
#include "drivers/pci/pci_header.hpp"
#include "drivers/apic/traps.h"

#include "data/List.h"

struct pci_device
{
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
	uint16_t seg;

	uint8_t* config;

	bool msi_support;
	bool msix_support;
	bool is_pcie;

	list_head list;

	[[nodiscard, maybe_unused]]
	uint32_t read_dword(size_t off) const
	{
		return (*(uint32_t*)(this->config + (off)));
	}

	template<typename TRegPtr>
	requires Pointer<TRegPtr>
	[[nodiscard, maybe_unused]]
	TRegPtr read_dword_as(size_t off) const
	{
		return (TRegPtr)(this->config + (off));
	}

	[[maybe_unused]]
	void write_dword(size_t off, uint32_t value)
	{
		(*(uint32_t*)(this->config + (off))) = (value);
	}

	bool operator==(const pci_device& rhs) const
	{
		return bus == rhs.bus &&
			dev == rhs.dev &&
			func == rhs.func &&
			seg == rhs.seg;
	}

	bool operator!=(const pci_device& rhs) const
	{
		return !(rhs == *this);
	}
};
