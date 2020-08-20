#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"

#include "libkernel/console/builtin_text_io.hpp"

using namespace pci;
using namespace pci::express;

using namespace libkernel;

struct dev_list
{
	list_head dev_head;
	size_t count;

	void add(pci_device* dev)
	{
		list_add(&dev->list, &dev_head);
		count++;
	}

	dev_list() : dev_head(list_head{}),
				 count(0)
	{
		list_init(&this->dev_head);
	}
};

dev_list pci_devices{};
list_head pci_msi_handle_head;

static inline pci_capability_reg* find_first_capability(uint8_t id,
	uintptr_t capability_ptr,
	IN const uint8_t* config)
{
	for (auto cap = (pci_capability_reg*)(config + capability_ptr);
		 cap->reg0.next_ptr != 0x00;
		 cap = (pci_capability_reg*)(config + cap->reg0.next_ptr))
	{
		if (cap->reg0.capability_id == id)
		{
			return cap;
		}
	}

	return nullptr;
}

static inline void pcie_config_device(pci_device* dev)
{
	pci_devices.add(dev);

	auto cmd_stat_reg = dev->read_dword_as<pci_command_status_reg*>(PCIE_HEADER_OFFSET_STATUS_COMMAND);

	dev->is_pcie = cmd_stat_reg->status.capabilities_list != 0;
	if (dev->is_pcie)
	{
		auto info_reg = dev->read_dword_as<pci_info_reg*>(PCIE_HEADER_OFFSET_INFO);

		// ignore the multi-function bit
		auto pure_header_type = info_reg->header_type & 0b01111111;

		if (pure_header_type == 0x0)
		{
			auto cap_reg = dev->read_dword_as<pci_t_capability_ptr_reg*>(PCIE_T0_HEADER_OFFSET_CAPABILITIES_PTR);

			dev->msi_support = find_first_capability(0x5, cap_reg->capability_ptr, dev->config) != nullptr;
			dev->msix_support = find_first_capability(0x10, cap_reg->capability_ptr, dev->config) != nullptr;
		}
	}
}

static inline void pcie_enumerate_device(uintptr_t base_address,
	uint16_t seg,
	uint8_t start_bus,
	uint8_t bus,
	uint8_t dev_no)
{

	for (uint8_t func = 0; func < 8; func++)
	{
		uintptr_t off = ((uint32_t)(bus - start_bus) << 20u) | ((uint32_t)dev_no << 15u) | ((uint32_t)func << 12u);
		uint8_t* config = (uint8_t*)P2V(base_address + off);
		uint32_t id = *(uint32_t*)config;

		if ((id & 0xFFFFu) != 0xFFFF)
		{
			pci_device* dev = new pci_device
				{
					.bus=bus,
					.dev=dev_no,
					.func=func,
					.seg=seg,
					.config=config
				};

			pcie_config_device(dev);
		}
	}
}

static inline void pcie_enumerate_entry(acpi::mcfg_entry entry)
{
	for (uint16_t bus = entry.start_pci_bus; bus < entry.end_pci_bus; bus++)
	{
		for (uint8_t dev = 0; dev < 32; dev++)
		{
			uintptr_t offset = ((bus - entry.start_pci_bus) << 20u) | (dev << 15u);
			uint8_t* config = (uint8_t*)P2V(entry.base_address + offset);

			if (((*(uint32_t*)config) & 0xFFFFu) != 0xFFFFu)
			{
				pcie_enumerate_device(entry.base_address, entry.pci_segment_group, entry.start_pci_bus, bus, dev);
			}
		}
	}
}

static inline void print_devices_debug_info()
{
	list_for_each(&pci_devices.dev_head, [](list_head* head)
	{
	  auto entry = list_entry(head, pci_device, list);

	  auto class_reg = entry->read_dword_as<pci_class_reg*>(PCIE_HEADER_OFFSET_CLASS);

	  if (entry->is_pcie)
	  {
		  write_format("  PCIe device %s MSI support : class 0x%x, subclass 0x%x, prog IF 0x%x\n",
			  entry->msi_support ? "with" : "without",
			  class_reg->class_code,
			  class_reg->subclass,
			  class_reg->prog_if);
	  }
	  else
	  {

		  write_format("  PCI device : class 0x%x, subclass 0x%x, prog IF 0x%x\n",
			  class_reg->class_code,
			  class_reg->subclass,
			  class_reg->prog_if);
	  }
	});
}

error_code pci::express::pcie_init(acpi::acpi_mcfg* mcfg)
{

	size_t entry_count =
		(mcfg->header.length - sizeof(acpi::acpi_desc_header) - sizeof(mcfg->reserved)) / sizeof(acpi::mcfg_entry);

	for (size_t i = 0; i < entry_count; i++)
	{
		pcie_enumerate_entry(mcfg->config_spaces[i]);
	}

	print_devices_debug_info();

	return ERROR_SUCCESS;
}

error_code pci::express::pcie_device_enable_msi(IN pci_device* dev)
{
	auto cap_reg = dev->read_dword_as<pci_t_capability_ptr_reg*>(PCIE_T0_HEADER_OFFSET_CAPABILITIES_PTR);
	pci_capability_reg* msi_capability = find_first_capability(0x05, cap_reg->capability_ptr, dev->config);
	// check msi capability
	if (msi_capability == nullptr)
	{
		return -ERROR_INVALID;
	}

	// We process MSI interrupt on the boot CPU
	constexpr uintptr_t addr = 0xFEE00000;

	if (msi_capability->reg0.msg_control.bit64)
	{
		msi_capability->reg1to3.regs64bit.reg1.msg_addr_low = addr & 0xFFFFFFFF;
		msi_capability->reg1to3.regs64bit.reg2.msg_addr_high = addr >> 32;
		msi_capability->reg1to3.regs64bit.reg3.msg_data = trap::TRAP_MSI_BASE;
	}
	else
	{
		msi_capability->reg1to3.regs32bit.reg1.msg_addr_low = addr & 0xFFFFFFFF;
		msi_capability->reg1to3.regs32bit.reg3.msg_data = trap::TRAP_MSI_BASE;
	}

	msi_capability->reg0.msg_control.multiple_msg_ena = 1;

	return ERROR_SUCCESS;
}