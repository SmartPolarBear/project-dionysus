#include "arch/amd64/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata.hpp"
#include "drivers/ahci/ata_string.hpp"

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

	uintptr_t fb_paddr = V2P((uintptr_t)(page_start + 8_KB));
	if ((fb_paddr % 4_KB))
	{
		return -ERROR_INVALID;
	}

	port->clb = cl_paddr;

	port->fb = fb_paddr;

	uint8_t* command_table_base = page_start + 16_KB;

	while (V2P((uintptr_t)command_table_base) % 128)
		command_table_base++;

	ahci_command_list_entry* cmd_list = (ahci_command_list_entry*)P2V(cl_paddr);
	for (size_t i = 0; i < AHCI_COMMAND_LIST_MAX; i++)
	{
		cmd_list->dw0.prdtl = 8;

		uintptr_t ctba_paddr = V2P((uintptr_t)(command_table_base + i * 256));

		if (ctba_paddr % 128)
		{
			return -ERROR_INVALID;
		}

		cmd_list->ctba = ctba_paddr;
	}

	ahci_port_start(port);

	return ERROR_SUCCESS;
}

static inline size_t ahci_port_find_free_cmd_slot(ahci_port* port)
{
	uint32_t slots = port->sact | port->ci;
	for (size_t i = 0; i < 32; i++)
	{
		if (!(slots | (i << 1)))
		{
			return i;
		}
	}

	return (size_t)-1;
}

static inline error_code ahci_port_identify([[maybe_unused]]ahci_controller* ctl, ahci_port* port)
{
	uint8_t cmd_id = 0;

	if (ctl->type == DEVICE_SATA)
	{
		cmd_id = ATA_CMD_IDENTIFY;
	}
	else if (ctl->type == DEVICE_SATAPI)
	{
		cmd_id = ATA_CMD_PACKET_IDENTIFY;
	}
	else
	{
		return -ERROR_INVALID;
	}

	uint16_t* buf = new uint16_t[256];
	if (buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	error_code ret = ERROR_SUCCESS;

	do
	{
		if ((ret = ahci_port_send_command(port, cmd_id, 0, buf, sizeof(uint16_t[256]))) != ERROR_SUCCESS)
		{
			kdebug::kdebug_log(kdebug::error_message(ret));
			break;
		}

		char* model_num = new char[40], * serial_num = new char[20];
		if (model_num == nullptr || serial_num == nullptr)
		{
			ret = -ERROR_MEMORY_ALLOC;
			break;
		}

		char* s_iter = serial_num;
		for (auto serial = &buf[ATA_IDENT_SERIAL_OFFSET];
			 serial != &buf[ATA_IDENT_SERIAL_OFFSET + ATA_IDENT_SERIAL_LEN_WORD];
			 serial++)
		{
			ahci::ATAStrWord word{ *serial };
			auto char_pair = word.get_char_pair();
			*(s_iter++) = char_pair.first;
			*(s_iter++) = char_pair.second;
		}
		*(s_iter++) = '\0'; // make the string null-terminated

		char* m_iter = model_num;
		for (auto model = &buf[ATA_IDENT_MODEL_OFFSET];
			 model != &buf[ATA_IDENT_MODEL_OFFSET + ATA_IDENT_MODEL_LEN_WORD];
			 model++)
		{
			ahci::ATAStrWord word{ *model };
			auto char_pair = word.get_char_pair();
			*(m_iter++) = char_pair.first;
			*(m_iter++) = char_pair.second;
		}
		*(m_iter++) = '\0'; // make the string null-terminated

		ata_ident_cmd_fea_supported* cmd_sets =
			reinterpret_cast<ata_ident_cmd_fea_supported*>(&buf[ATA_IDENT_CMD_SUP_OFFSET]);

		uint32_t lsec_size =
			*((uint32_t*)&buf[ATA_IDENT_LOGICAL_SECTOR_SIZE_OFFSET]);

		size_t disk_size = 0;
		if (cmd_sets->lba48)
		{
			uint64_t lba48_sectors = *((uint64_t*)&buf[ATA_IDENT_LBA48_TOTAL_SECTORS_OFFSET]);
			disk_size = lba48_sectors * lsec_size;
		}
		else
		{
			uint64_t lba28_sectors = *((uint64_t*)&buf[ATA_IDENT_LBA28_TOTAL_SECTORS_OFFSET]);
			disk_size = lba28_sectors * lsec_size;
		}

		// report disk information

		kdebug::kdebug_log("%s Disk: capacity %lld bytes, serial %s, model %s\n",
			ctl->type == DEVICE_SATA ? "ATA" : "ATAPI",
			disk_size,
			serial_num,
			model_num);

		delete[] serial_num;
		delete[] model_num;

	} while (0);

	delete[] buf;

	return ret;
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
	memset(cmd_table, 0, sizeof(ahci_command_table) + sizeof(ahci_prd) * prd_count);

	ahci_fis_reg_h2d* fis = &cmd_table->fis_h2d;
	memset(fis, 0, sizeof(ahci_fis_reg_h2d));

	fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
	fis->command = cmd_id;
	fis->c = 1;

	for (size_t i = 0, bytes_left = sz; i < prd_count; i++)
	{
		size_t prd_size = min(AHCI_PRD_MAX_SIZE, sz);
		if (prd_size <= 0)
		{
			return -ERROR_SUCCESS;
		}

		uintptr_t prd_buf_paddr = V2P((uintptr_t)data) + i * AHCI_PRD_MAX_SIZE;
		cmd_table->prdt[i].dba = prd_buf_paddr & 0xFFFFFFFF;
		cmd_table->prdt[i].dbau = prd_buf_paddr >> 32;
		cmd_table->prdt[i].dw3.dbc = prd_size - 1u;

		if (i == prd_count - 1)
		{
			cmd_table->prdt[i].dw3.interrupt_on_completion = 0;
		}

		bytes_left -= prd_size;
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

		if (!(port->ci & (1u << slot)))
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