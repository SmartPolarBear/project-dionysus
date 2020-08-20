#pragma once
#include "system/types.h"
#include "system/concepts.hpp"

#include "drivers/acpi/acpi.h"
#include "drivers/pci/pci_header_common.hpp"
#include "drivers/pci/pci_header_t0.hpp"
#include "drivers/pci/pci_header_t1.hpp"
#include "drivers/pci/pci_header_t2.hpp"
#include "drivers/apic/traps.h"

#include "data/List.h"

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
		PCI_CAPABILITY_ID_MSI = 0x5
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

	struct pci_device
	{
		uint8_t bus;
		uint8_t dev;
		uint8_t func;
		uint16_t seg;

		uint8_t* config;

		bool msi_support;
		bool is_pcie;

		list_head list;

		[[nodiscard, maybe_unused]]
		uint32_t read_dword(size_t off) const
		{
			return (*(uint32_t*)(this->config + (off)));
		}

		template<typename TRegPtr>
		requires Pointer<TRegPtr>
		[[nodiscard]] TRegPtr read_dword_as(size_t off) const
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

		error_code pcie_initialize_msi(IN pci_device* dev, trap::trap_handle int_handle);
	}

	// initialize PCI and PCIe
	void pci_init();
}