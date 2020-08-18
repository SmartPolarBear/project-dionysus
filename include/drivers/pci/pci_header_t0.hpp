#pragma once

#include "system/types.h"

#include "drivers/acpi/acpi.h"

#include "data/List.h"

namespace pci
{
	namespace express
	{
		constexpr size_t PCIE_T0_HEADER_OFFSET_BAR(size_t n)
		{
			return 0x10 + sizeof(uint32_t) * n;
		}

		constexpr size_t PCIE_T0_HEADER_OFFSET_CARDBUS_CIS_PTR = 0x28;
		constexpr size_t PCIE_T0_HEADER_OFFSET_SUBSYS_ID = 0x2C;
		constexpr size_t PCIE_T0_HEADER_OFFSET_EXPANSION_ROM_BASE_ADDR = 0x30;
		constexpr size_t PCIE_T0_HEADER_OFFSET_CAPABILITIES_PTR = 0x34;
		constexpr size_t PCIE_T0_HEADER_OFFSET_INTR = 0x3C;

		struct pcie_t_capability_ptr_reg
		{
			uint8_t capability_ptr;
			uint64_t reserved: 24;
		}__attribute__((__packed__));
		static_assert(sizeof(pcie_t_capability_ptr_reg) == sizeof(uint32_t));

	}
}