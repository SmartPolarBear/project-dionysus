#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "libkernel/console/builtin_text_io.hpp"

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

static inline error_code ahci_config_port(ahci_controller* ctl, ahci_port* port, size_t no)
{
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
			port->sig.lba_low == 1&&
			port->sig.lba_mid == 0x0 &&
			port->sig.lba_high == 0x0)    // SATA
		{
			kdebug::kdebug_log("Configure SATA speed generation %d\n", port->ssts.spd);
			ret = ahci_config_port(ctl, port, i);
		}
		else if (port->sig.sec_count_reg == 0x01 &&
			port->sig.lba_low == 0x01 &&
			port->sig.lba_mid == 0x14 &&
			port->sig.lba_high == 0xEB)   // SATAPI
		{
			kdebug::kdebug_log("Configure SATAPI speed generation %d\n", port->ssts.spd);
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

	kdebug::kdebug_log("Initialize AHCI %d.%d\n", ghc->vs.major_ver, ghc->vs.minor_ver);

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