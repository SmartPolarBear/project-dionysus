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

		struct pci_capability_reg
		{
			pci_capability_reg0 reg0;
			pci_capability_reg1 reg1;
			pci_capability_reg2 reg2;
			pci_capability_reg3 reg3;
			pci_capability_reg4 reg4;
		}__attribute__((__packed__));
		static_assert(sizeof(pci_capability_reg) == sizeof(uint32_t) * 5);

		struct pci_device
		{
			uint8_t bus, dev, func;

			uint16_t segment_group;
			uint8_t* config;

			uint8_t* capability_list;

			list_head list;

			[[nodiscard]] uint32_t read_dword(size_t off) const
			{
				return (*(uint32_t*)(this->config + (off)));
			}

			void write_dword(size_t off, uint32_t value)
			{
				(*(uint32_t*)(this->config + (off))) = (value);
			}
		};

		error_code pcie_init(acpi::acpi_mcfg* mcfg);
	}

	// initialize PCI and PCIe
	void pci_init();
}