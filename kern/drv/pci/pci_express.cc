#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"

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

enum DEV_SUPPORT
{
	DS_UNSUPPORTED, DS_SUPPORTED
};

dev_list supported{};
dev_list unsupported{};
size_t dev_total = 0;

static inline const pci_capability_reg* find_first_capability(uint8_t id, const pci_device* dev)
{
	for (auto capa = (pci_capability_reg*)(dev->capability_list);
		 capa->reg0.next_ptr != 0x00;
		 capa = (pci_capability_reg*)(dev->config + capa->reg0.next_ptr))
	{
		if (capa->reg0.capability_id == id)
		{
			return capa;
		}
	}

	return nullptr;
}

static inline bool check_msi_capability(const pci_device* dev)
{
	auto msi_capability = find_first_capability(PCI_CAPABILITY_ID_MSI, dev);
	return msi_capability != nullptr;
}

static inline DEV_SUPPORT pcie_device_init(pci_device* dev)
{
	DEV_SUPPORT ret = DS_UNSUPPORTED;

	auto cmd_stat_reg_val = dev->read_dword(PCIE_HEADER_OFFSET_STATUS_COMMAND);
	auto cmd_stat_reg = *((pci_command_status_reg*)(&cmd_stat_reg_val));

	if (cmd_stat_reg.status.capabilities_list == 0x1)
	{
		auto info_reg_val = dev->read_dword(PCIE_HEADER_OFFSET_OTHER_INFO);
		auto info_reg = *((pci_info_reg*)(&info_reg_val));

		switch (info_reg.header_type)
		{
		default:
		{
			ret = DS_UNSUPPORTED;
			break;
		}
		case 0x0:
		{
			auto capa_reg_val = dev->read_dword(PCIE_T0_HEADER_OFFSET_CAPABILITIES_PTR);
			auto capa_reg = *((pci_t_capability_ptr_reg*)(&capa_reg_val));

			// update capability_list for the device !!!
			dev->capability_list = dev->config + capa_reg.capability_ptr;

			if (check_msi_capability(dev))
			{
				ret = DS_SUPPORTED;
			}

			break;
		}
		case 0x1:
		{
			ret = DS_UNSUPPORTED;
			break;
		}
		case 0x2:
		{
			ret = DS_UNSUPPORTED;
			break;
		}
		}
	}
	else
	{
		ret = DS_UNSUPPORTED;
	}

	return ret;
}

static inline void pcie_enumerate_device(uintptr_t base_address,
	uint16_t seg,
	uint8_t start_bus,
	uint8_t bus,
	uint8_t dev_no)
{

	for (uint8_t func = 0; func < 8; func++)
	{
		uint32_t d = ((uint32_t)(bus - start_bus) << 20u) | ((uint32_t)dev_no << 15u) | ((uint32_t)func << 12u);
		uint8_t* config = (uint8_t*)P2V(base_address + d);
		uint32_t id = *(uint32_t*)config;

		if ((id & 0xFFFFu) == 0xFFFF)
		{
			continue;
		}

		pci_device* dev = new pci_device;

		KDEBUG_ASSERT(dev != nullptr);

		dev->bus = bus;
		dev->dev = dev_no;
		dev->func = func;
		dev->segment_group = seg;
		dev->config = config;

		auto sup = pcie_device_init(dev);

		if (sup == DS_SUPPORTED)
		{
			supported.add(dev);
		}
		else if (sup == DS_UNSUPPORTED)
		{
			unsupported.add(dev);
		}
	}
}

static inline void pcie_enumerate_entry(acpi::mcfg_entry entry)
{
	for (uint16_t bus = entry.start_pci_bus; bus < entry.end_pci_bus; bus++)
	{
		for (uint8_t dev = 0; dev < 32; dev++)
		{
			uint8_t* config =
				(uint8_t*)P2V(entry.base_address + ((((uint8_t)(bus - entry.start_pci_bus)) << 20u) | (dev << 15u)));

			if (((*(uint32_t*)config) & 0xFFFFu) != 0xFFFFu)
			{
				uint32_t header = *(uint32_t*)(config + 0x0C);
				header >>= 16u;
				header &= 0x7Fu;

				if (header == 0x00)
				{
					pcie_enumerate_device(entry.base_address, entry.pci_segment_group, entry.start_pci_bus, bus, dev);
				}
				else
				{
					kdebug::kdebug_warning("Skipping unsupported header type: %02x\n", header);
				}

			}
		}
	}
}

static inline void print_devices_debug_info()
{

	write_format("PCIe bus has %d supported device(s), %d unsupported device(s):\n",
		supported.count,
		unsupported.count);

	list_for_each(&supported.dev_head, [](list_head* head)
	{
	  auto entry = list_entry(head, pci_device, list);
	  auto class_reg_val = entry->read_dword(PCIE_HEADER_OFFSET_CLASS);
	  auto class_reg = (pci_class_reg*)(&class_reg_val);
	  write_format("  Supported device : class 0x%x, subclass 0x%x, prog IF 0x%x\n",
		  class_reg->class_code,
		  class_reg->subclass,
		  class_reg->prog_if);
	});

	list_for_each(&unsupported.dev_head, [](list_head* head)
	{
	  auto entry = list_entry(head, pci_device, list);
	  auto class_reg_val = entry->read_dword(PCIE_HEADER_OFFSET_CLASS);
	  auto class_reg = (pci_class_reg*)(&class_reg_val);
	  write_format("  Unsupported device : class 0x%x, subclass 0x%x, prog IF 0x%x\n",
		  class_reg->class_code,
		  class_reg->subclass,
		  class_reg->prog_if);
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

	dev_total = supported.count + unsupported.count;

	print_devices_debug_info();

	return ERROR_SUCCESS;
}

error_code pcie_device_initialize_msi(IN pci_device* dev, trap::trap_handle int_handle)
{
	// check msi capability
	if (!check_msi_capability(dev))
	{
		return -ERROR_INVALID;
	}

	// We process MSI interrupt on the boot CPU

	auto msi_capability = const_cast<pci_capability_reg*>(find_first_capability(PCI_CAPABILITY_ID_MSI, dev));

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