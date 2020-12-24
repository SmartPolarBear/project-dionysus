#pragma once
#include "system/types.h"

#include "drivers/acpi/acpi.h"

#include "kbl/pod_list.h"

namespace pci
{

	// Common header fields
	/*	register	offset	bits 31-24	bits 23-16	bits 15-8		bits 7-0
		0			0		Device ID				Vendor ID
		1			4		Status					Command
		2			8		Class code	Subclass	Prog IF			Revision ID
		3			0C		BIST		Header type	Latency Timer	Cache Line Size
	 */

	constexpr size_t PCIE_HEADER_OFFSET_ID = 0x0;
	constexpr size_t PCIE_HEADER_OFFSET_STATUS_COMMAND = 0x4;
	constexpr size_t PCIE_HEADER_OFFSET_CLASS = 0x8;
	constexpr size_t PCIE_HEADER_OFFSET_INFO = 0x0C;

	static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);

//		struct test
//		{
//			uint16_t low;
//			uint16_t high;
//		};

	struct pci_status_reg
	{
		uint64_t reserved0: 3;
		uint64_t int_status: 1;
		uint64_t capabilities_list: 1;    // pcie always 1
		uint64_t capable_66MHZ: 1;    // pcie always 0
		uint64_t reserved1: 1;
		uint64_t fast_b2b_capable: 1; //fast back to back , pcie always 0
		uint64_t master_data_parity_err: 1;
		uint64_t DEVSEL_timing: 2; // pcie always 0
		uint64_t signaled_target_abort: 1;
		uint64_t received_target_abort: 1;
		uint64_t received_master_abort: 1;
		uint64_t signaled_sys_error: 1;
		uint64_t detected_parity_error: 1;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_status_reg) == sizeof(uint16_t), "Status reg has 16 bits.");

	struct pci_command_reg
	{
		uint64_t io_space: 1;
		uint64_t mem_space: 1;
		uint64_t bus_master: 1;
		uint64_t special_cycles: 1;  // pcie always 0
		uint64_t mem_write_invalidate_enable: 1;  // pcie always 0
		uint64_t vga_palette_snoop: 1;  // pcie always 0
		uint64_t parity_err_response: 1;
		uint64_t reserved0: 1;  // pcie always 0
		uint64_t SERR_enable: 1;
		uint64_t fast_b2b_enable: 1; //fast back to back , pcie always 0
		uint64_t reserved1: 5;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_command_reg) == sizeof(uint16_t), "Command reg has 16 bits.");

	struct pci_command_status_reg
	{
		pci_command_reg command;
		pci_status_reg status;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_command_status_reg) == sizeof(uint32_t));

	struct pci_id_reg
	{
		uint16_t vendor_id;
		uint16_t dev_id;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_id_reg) == sizeof(uint32_t));

	struct pci_class_reg
	{
		uint8_t rev_id;
		uint8_t programming_interface;
		uint8_t subclass;
		uint8_t class_code;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_class_reg) == sizeof(uint32_t));

	struct pci_info_reg
	{
		uint8_t cache_line_size;
		uint8_t latency_timer;
		uint8_t header_type;
		uint8_t BIST;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_info_reg) == sizeof(uint32_t));


	// Header type 0

	constexpr size_t PCIE_T0_HEADER_OFFSET_BAR(size_t n)
	{
		return 0x10 + sizeof(uint32_t) * n;
	}

	constexpr size_t PCIE_T0_HEADER_OFFSET_CARDBUS_CIS_PTR = 0x28;
	constexpr size_t PCIE_T0_HEADER_OFFSET_SUBSYS_ID = 0x2C;
	constexpr size_t PCIE_T0_HEADER_OFFSET_EXPANSION_ROM_BASE_ADDR = 0x30;
	constexpr size_t PCIE_T0_HEADER_OFFSET_CAPABILITIES_PTR = 0x34;
	constexpr size_t PCIE_T0_HEADER_OFFSET_INTR = 0x3C;

	struct pci_t_capability_ptr_reg
	{
		uint8_t capability_ptr;
		uint64_t reserved: 24;
	}__attribute__((__packed__));
	static_assert(sizeof(pci_t_capability_ptr_reg) == sizeof(uint32_t));
}
