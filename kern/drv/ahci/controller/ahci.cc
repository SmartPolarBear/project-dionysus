#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

using namespace pci;
using namespace pci::express;

using namespace ahci;

using namespace libkernel;

using std::min;
using std::max;

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

	port->cmd.fre = 1;
	port->cmd.st = 1;
}

__attribute__((always_inline))
static inline void ahci_port_stop(ahci_port* port)
{
	port->cmd.fre = 0;
	port->cmd.st = 0;

	while (port->cmd.cr || port->cmd.fr)
	{
		asm("pause");
	}
}

__attribute__((always_inline))
static inline ahci_command_list_entry* ahci_port_cmd_list(const ahci_port* port)
{
	return (ahci_command_list_entry*)P2V(port->clb);
}

__attribute__((always_inline))
static inline ahci_command_table* ahci_cmd_entry_table(const ahci_command_list_entry* entry)
{
	return (ahci_command_table*)P2V(entry->ctba);
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

	auto vmm_pg = vmm::walk_pgdir(vmm::g_kpml4t, pmm::page_to_va(page), false);
	*vmm_pg |= PG_PWT;
	*vmm_pg |= PG_PCD;
	pmm::tlb_invalidate(vmm::g_kpml4t, pmm::page_to_va(page));

	memset(reinterpret_cast<void*>(pmm::page_to_va(page)), 0, PAGE_SIZE);

	// command list , FIS buffer, command table should be 4K aligned
	uint8_t* page_start = reinterpret_cast<uint8_t*>(pmm::page_to_va(page));

	uintptr_t cl_paddr = V2P((uintptr_t)page_start);
	if ((cl_paddr % 1_KB))
	{
		return -ERROR_INVALID;
	}

	port->clb = cl_paddr;

	uintptr_t fb_paddr = V2P((uintptr_t)(page_start + 4_KB));
	if ((fb_paddr % 4_KB))
	{
		return -ERROR_INVALID;
	}

	port->fb = fb_paddr;

	uint8_t* command_table_base = page_start + 8_KB;
	ahci_command_list_entry* cmd_list = (ahci_command_list_entry*)P2V(cl_paddr);
	for (size_t i = 0; i < AHCI_COMMAND_LIST_MAX; i++)
	{
		cmd_list[i].dw0.prdtl = 8;

		uintptr_t ctba_paddr = V2P((uintptr_t)(command_table_base)) + i * 256;

		if (ctba_paddr % 128)
		{
			return -ERROR_INVALID;
		}

		cmd_list[i].ctba = ctba_paddr;
	}

	ahci_port_start(port);

	return ERROR_SUCCESS;
}

__attribute__((always_inline))
static inline size_t ahci_port_find_free_cmd_slot(ahci_port* port)
{
	uint32_t slots = port->sact | port->ci;
	for (size_t i = 0; i < 32; i++)
	{
		if ((slots & (1U << i)) == 0)
		{
			return i;
		}
	}

	return (size_t)-1;
}

__attribute__((always_inline))
static inline error_code make_prd(ahci_prd* prd, uintptr_t addr, size_t sz)
{
	if (sz > 4 * 1024 * 1024)
	{
		return -ERROR_INVALID;
	}

	prd->dba = addr & 0xFFFFFFFF;
	prd->dbau = (addr >> 32) & 0xFFFFFFFF;

	prd->dw3.dbc = ((sz - 1u) << 1u) | 1u;

	return ERROR_SUCCESS;
}

static inline error_code ahci_port_identify([[maybe_unused]]ahci_controller* ctl, ahci_port* port)
{
	if (ctl->type == DEVICE_SATA)
	{
		return ata_identify_device(port);
	}
	else if (ctl->type == DEVICE_SATAPI)
	{
		return atapi_identify_device(port);
	}
	else
	{
		return -ERROR_INVALID;
	}
}

static inline error_code ahci_config_port([[maybe_unused]]ahci_controller* ctl, ahci_port* port)
{
	error_code ret = ahci_port_allocate(port);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	ret = ahci_port_identify(ctl, port);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

static inline error_code ahci_enumerate_all_ports(ahci_controller* ctl, ahci_hba_mem* hba)
{
	for (size_t i = 0; i < 32; i++)
	{
		// test the bit, if set, the port is implemented
		if ((hba->ghc.pi.bits & (1 << i)) == 0)
		{
			continue;
		}

		if (hba->ports[i].ssts.ipm != IPM_ACTIVE || hba->ports[i].ssts.det != DET_DEV_AND_COMM)
		{
			continue;
		}

		error_code ret = ERROR_SUCCESS;

		if (hba->ports[i].sig.sec_count_reg == 1 &&
			hba->ports[i].sig.lba_low == 1 &&
			hba->ports[i].sig.lba_mid == 0x0 &&
			hba->ports[i].sig.lba_high == 0x0)    // SATA
		{
			kdebug::kdebug_log("Configuring SATA at generation %d's speed.\n", hba->ports[i].ssts.spd);
			ctl->type = DEVICE_SATA;
			ret = ahci_config_port(ctl, &hba->ports[i]);
		}
		else if (hba->ports[i].sig.sec_count_reg == 0x01 &&
			hba->ports[i].sig.lba_low == 0x01 &&
			hba->ports[i].sig.lba_mid == 0x14 &&
			hba->ports[i].sig.lba_high == 0xEB)   // SATAPI
		{
			kdebug::kdebug_log("Configure SATAPI at generation %d's speed.\n", hba->ports[i].ssts.spd);
			ctl->type = DEVICE_SATAPI;
			ret = ahci_config_port(ctl, &hba->ports[i]);
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
	ahci_hba_mem* hba = (ahci_hba_mem*)ctl->regs;

	bool valid_major = hba->ghc.vs.major_ver == 0 || hba->ghc.vs.major_ver == 1;

	bool valid_minor = hba->ghc.vs.minor_ver == 0 ||
		hba->ghc.vs.minor_ver == 10 ||
		hba->ghc.vs.minor_ver == 20 ||
		hba->ghc.vs.minor_ver == 30 ||
		hba->ghc.vs.minor_ver == 31 ||
		hba->ghc.vs.minor_ver == 95;

	if ((!valid_minor) || (!valid_major))
	{
		return -ERROR_INVALID;
	}

	kdebug::kdebug_log("Initialize AHCI controller (ver %d.%d)\n", hba->ghc.vs.major_ver, hba->ghc.vs.minor_ver);

	auto ret = ahci_enumerate_all_ports(ctl, hba);

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

error_code ahci::ahci_port_send_command(ahci_port* port,
	uint8_t cmd_id,
	uintptr_t lba,
	void* data,
	size_t sz)
{
	auto slot = ahci_port_find_free_cmd_slot(port);

	if (slot > 31)
	{
		return -ERROR_DEV_BUSY;
	}

	auto cl_entry = &ahci_port_cmd_list(port)[slot];
	auto cmd_table = ahci_cmd_entry_table(cl_entry);

	size_t nsect = (sz + (ATA_DEFAULT_SECTOR_SIZE - 1)) / ATA_DEFAULT_SECTOR_SIZE;

	if ((nsect & ~0xFFFFu) != 0)
	{
		return -ERROR_INVALID;
	}

	size_t prd_count = (sz + ahci::AHCI_PRD_MAX_SIZE - 1) / ahci::AHCI_PRD_MAX_SIZE;

	ahci_fis_reg_h2d* fis = &cmd_table->fis_reg_h2d;
	memset(fis, 0, sizeof(ahci_fis_reg_h2d));

	fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
	fis->command = cmd_id;
	fis->c = 1;

	for (size_t i = 0, bytes_left = sz, prd_size = min(AHCI_PRD_MAX_SIZE, bytes_left);
		 i < prd_count;
		 i++, bytes_left -= prd_size)
	{
		if (prd_size <= 0)
		{
			return -ERROR_SUCCESS;
		}
		uintptr_t prd_buf_paddr = V2P((uintptr_t)data) + i * AHCI_PRD_MAX_SIZE;

		make_prd(&cmd_table->prdt[i], prd_buf_paddr, prd_size);

		if (i == prd_count - 1)
		{
			cmd_table->prdt[i].dw3.interrupt_on_completion = 1;
		}
	}

	cl_entry->dw0.cfl = sizeof(ahci_fis_reg_h2d) / sizeof(uint32_t);
	cl_entry->dw0.prdtl = prd_count;
	cl_entry->prdbc = 0;

	if (cmd_id != AHCI_ATA_CMD_IDENTIFY)
	{
		fis->device = 1u << 6u; //LBA mode

		fis->lba0 = lba & 0xFFu;
		fis->lba1 = (lba >> 8u) & 0xFFu;
		fis->lba2 = (lba >> 16u) & 0xFFu;
		fis->lba3 = (lba >> 24u) & 0xFFu;
		fis->lba4 = (lba >> 32u) & 0xFFu;
		fis->lba5 = (lba >> 40u) & 0xFFu;

		fis->countl = nsect & 0xFFFFu;
	}

	// Wait for port to be free for commands
	size_t spin = 0;
	while ((port->tfd.sts_bsy || port->tfd.sts_drq) && spin < AHCI_SPIN_WAIT_MAX)
	{
		asm volatile ("pause");
		++spin;
	}

	if (spin == AHCI_SPIN_WAIT_MAX)
	{
		kdebug::kdebug_log("AHCI: Device hang.\n");
		return -ERROR_DEV_TIMEOUT;
	}

	port->ci |= (1u << slot);

	while (true)
	{
		asm volatile("nop");

		if (((port->sact | port->ci) & (1u << slot)) == 0 && port->is.dps == 0)
		{
			break;
		}

		if (port->is.tfes)
		{
			kdebug::kdebug_log("AHCI: Task file error signalled.\n");
			return -ERROR_IO;
		}
	}

	if (port->is.tfes)
	{
		kdebug::kdebug_log("AHCI: Task file error signalled.\n");
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}