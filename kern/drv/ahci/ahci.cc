#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>

using namespace pci;
using namespace pci::express;

using namespace ahci;

using namespace libkernel;

struct pci_achi_devices_struct
{
	list_head dev_head{};
	size_t count{};

	void add(ahci_controller* dev)
	{
		count++;
		list_add(&dev->list, &dev_head);
	}

	pci_achi_devices_struct()
	{
		list_init(&dev_head);
	}
} ahci_devs;

__attribute__((always_inline))
static inline void ahci_port_start(ahci_port* port)
{
	while (port->cmd.cr)
	{
		asm("pause");
	}

	port->cmd.fre = true;
	port->cmd.st = true;
}

__attribute__((always_inline))
static inline void ahci_port_stop(ahci_port* port)
{
	port->cmd.fre = false;
	port->cmd.st = false;

	while (port->cmd.cr || port->cmd.fr)
	{
		asm("pause");
	}
}

static inline error_code ahci_port_allocate(ahci_port* port)
{
	ahci_port_stop(port);

	// 2MB-sized page
	auto page = pmm::alloc_page();
	if (page == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	memset(reinterpret_cast<void*>(pmm::page_to_va(page)), 0, PAGE_SIZE);

	// command list , FIS buffer, command table should be 4K aligned
	uint8_t* page_start = reinterpret_cast<uint8_t*>(pmm::page_to_va(page));

	uintptr_t cl_paddr = V2P((uintptr_t)page_start);
	if ((cl_paddr % 1_KB))
	{
		return -ERROR_INVALID;
	}

	uintptr_t fb_paddr = V2P((uintptr_t)(page_start + 4_KB));
	if ((fb_paddr % 4_KB))
	{
		return -ERROR_INVALID;
	}

	port->clb = cl_paddr & 0xFFFFFFFF;
	port->clbu = cl_paddr >> 32;

	port->fb = fb_paddr & 0xFFFFFFFF;
	port->fbu = fb_paddr >> 32;

	uint8_t* command_table_base = page_start + 8_KB;
	while (V2P((uintptr_t)command_table_base) % 128)command_table_base++;

	ahci_command_list* cmd_list = (ahci_command_list*)P2V(cl_paddr);
	for (size_t i = 0; i < AHCI_COMMAND_LIST_MAX; i++)
	{
		cmd_list->dw0.prdtl = 8;

		uintptr_t ctba_paddr = V2P((uintptr_t)(command_table_base + i * 256));

		if (ctba_paddr % 128)
		{
			return -ERROR_INVALID;
		}

		cmd_list->ctba = ctba_paddr & 0xFFFFFFFF;
		cmd_list->ctba_u0 = ctba_paddr >> 32;
	}

	ahci_port_start(port);

	return ERROR_SUCCESS;
}

static inline error_code ahci_config_port(ahci_controller* ctl, ahci_port* port, size_t no)
{
	error_code ret = ahci_port_allocate(port);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

static inline error_code ahci_enumerate_all_ports(ahci_controller* ctl, ahci_generic_host_control* ghc)
{
	for (size_t i = 0; i < 32; i++)
	{
		// test the bit, if set, the port is implemented
		if ((ghc->pi.bits & (1 << i)) == 0)
		{
			continue;
		}

		uintptr_t offset = 0x100 + (i * 0x80);
		ahci_port* port = (ahci_port*)(ctl->regs + offset);

		if (port->ssts.ipm != IPM_ACTIVE || port->ssts.det != DET_DEV_AND_COMM)
		{
			continue;
		}

		error_code ret = ERROR_SUCCESS;

		if (port->sig.sec_count_reg == 1 &&
			port->sig.lba_low == 1 &&
			port->sig.lba_mid == 0x0 &&
			port->sig.lba_high == 0x0)    // SATA
		{
			kdebug::kdebug_log("Configuring SATA at generation %d's speed.\n", port->ssts.spd);
			ret = ahci_config_port(ctl, port, i);
		}
		else if (port->sig.sec_count_reg == 0x01 &&
			port->sig.lba_low == 0x01 &&
			port->sig.lba_mid == 0x14 &&
			port->sig.lba_high == 0xEB)   // SATAPI
		{
			kdebug::kdebug_log("Configure SATAPI at generation %d's speed.\n", port->ssts.spd);
			ret = ahci_config_port(ctl, port, i);
		}

		if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
	}
	return ERROR_SUCCESS;
}

static inline error_code ahci_initialize_controller(ahci_controller* ctl)
{
	ahci_generic_host_control* ghc = (ahci_generic_host_control*)ctl->regs;

	bool valid_major = ghc->vs.major_ver == 0 || ghc->vs.major_ver == 1;

	bool valid_minor = ghc->vs.minor_ver == 0 ||
		ghc->vs.minor_ver == 10 ||
		ghc->vs.minor_ver == 20 ||
		ghc->vs.minor_ver == 30 ||
		ghc->vs.minor_ver == 31 ||
		ghc->vs.minor_ver == 95;

	if ((!valid_minor) || (!valid_major))
	{
		return -ERROR_INVALID;
	}

	kdebug::kdebug_log("Initialize AHCI controller (ver %d.%d)\n", ghc->vs.major_ver, ghc->vs.minor_ver);

	auto ret = ahci_enumerate_all_ports(ctl, ghc);

	return ret;
}

error_code ahci::ahci_init()
{
	auto find_pred = [](const pci_device* dev)
	{

	  auto class_reg = dev->read_dword_as<pci_class_reg*>(PCIE_HEADER_OFFSET_CLASS);
	  auto info_reg = dev->read_dword_as<pci_info_reg*>(PCIE_HEADER_OFFSET_INFO);

	  return AHCI_PCI_CLASS == class_reg->class_code &&
		  AHCI_PCI_SUBCLASS == class_reg->subclass &&
		  AHCI_PCI_PROGIF == class_reg->programming_interface &&
		  (info_reg->header_type & 0b01111111u) == 0x0 &&
		  (dev->msi_support || dev->msix_support);
	};

	size_t count = pcie_find_devices(find_pred, 0, nullptr);

	auto devs = new pci_device[count];
	pcie_find_devices(find_pred, count, devs);

	for (size_t i = 0; i < count; i++)
	{
		ahci_abar* abar = devs[i].read_dword_as<ahci_abar*>(PCIE_T0_HEADER_OFFSET_BAR(5));
		if (abar->res_type_indicator == 0)
		{
			ahci_controller* ahci = new ahci_controller
				{
					.pci_dev=&devs[i],
					.regs=(uint8_t*)P2V(devs[i].read_dword(PCIE_T0_HEADER_OFFSET_BAR(5)))
				};

			if (ahci_initialize_controller(ahci) == ERROR_SUCCESS)
			{
				ahci_devs.add(ahci);
			}
			else
			{
				delete ahci;
			}
		}
	}

	return ERROR_SUCCESS;
}