#pragma once
#include "system/types.h"
#include "system/concepts.hpp"

#include "drivers/acpi/acpi.h"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/apic/traps.h"

#include "kbl/pod_list.h"

namespace pci
{

	struct pci_message_control_reg
	{
		uint64_t enable: 1;
		uint64_t multiple_msg_cap: 3;
		uint64_t multiple_msg_ena: 3;
		uint64_t bit64: 1;
		uint64_t reserved: 8;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_message_control_reg) == sizeof(uint16_t));

	struct pci_capability_reg0
	{
		uint8_t capability_id;
		uint8_t next_ptr;
		pci_message_control_reg msg_control;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg0) == sizeof(uint32_t));

	struct pci_capability_reg1
	{
		uint32_t msg_addr_low;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg1) == sizeof(uint32_t));

	struct pci_capability_reg2
	{
		uint32_t msg_addr_high;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg2) == sizeof(uint32_t));

	struct pci_capability_reg3
	{
		uint16_t msg_data;
		uint16_t reserved;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg3) == sizeof(uint32_t));

	struct pci_capability_reg4
	{
		uint32_t mask;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg4) == sizeof(uint32_t));

	enum capability_ids : uint8_t
	{
		PCI_CAPABILITY_ID_MSI = 0x5,
		PCI_CAPABILITY_ID_PCIE = 0x10,
		PCI_CAPABILITY_ID_MSIX = 0x11,
	};

	struct pci_capability_reg
	{
		pci_capability_reg0 reg0;

		union reg1to3_union
		{
			struct struct64bit
			{
				pci_capability_reg1 reg1;
				pci_capability_reg2 reg2;
				pci_capability_reg3 reg3;
			}__attribute__((__packed__)) regs64bit;
			struct struct32bit
			{
				pci_capability_reg1 reg1;
				pci_capability_reg3 reg3;
			}__attribute__((__packed__)) regs32bit;
		}__attribute__((__packed__)) reg1to3;

		pci_capability_reg4 reg4;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_capability_reg) == sizeof(uint32_t) * 5);
}
