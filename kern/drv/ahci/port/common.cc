#include "common.hpp"

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
#include "drivers/ahci/atapi/atapi.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ahci;

using std::min;

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



error_code common_identify_device(ahci_port* port, bool atapi)
{
	uint16_t* identify_buf = new uint16_t[256];

	char model_num[40] = {};
	char serial_num[20] = {};

	error_code ret = ERROR_SUCCESS;

	auto cmd = atapi ? ATA_CMD_PACKET_IDENTIFY : ATA_CMD_IDENTIFY;

	do
	{
		if ((ret = ahci_port_send_command(port, cmd, atapi, 0, identify_buf, sizeof(uint16_t[256])))
			!= ERROR_SUCCESS)
		{
			kdebug::kdebug_log(kdebug::error_message(ret));
			break;
		}

		char* s_iter = serial_num;
		for (auto serial = &identify_buf[ATA_IDENT_SERIAL_OFFSET];
			 serial != &identify_buf[ATA_IDENT_SERIAL_OFFSET + ATA_IDENT_SERIAL_LEN_WORD];
			 serial++)
		{
			ahci::ATAStrWord word{ *serial };
			auto char_pair = word.get_char_pair();
			if (char_pair.first != ' ')
			{
				*(s_iter++) = char_pair.first;
			}
			if (char_pair.first != ' ')
			{
				*(s_iter++) = char_pair.second;
			}
		}

		*(s_iter++) = '\0'; // make the string null-terminated

		char* m_iter = model_num;
		for (auto model = &identify_buf[ATA_IDENT_MODEL_OFFSET];
			 model != &identify_buf[ATA_IDENT_MODEL_OFFSET + ATA_IDENT_MODEL_LEN_WORD];
			 model++)
		{
			ahci::ATAStrWord word{ *model };
			auto char_pair = word.get_char_pair();
			if (char_pair.first != ' ')
			{
				*(m_iter++) = char_pair.first;
			}
			if (char_pair.first != ' ')
			{
				*(m_iter++) = char_pair.second;
			}
		}

		*(m_iter++) = '\0'; // make the string null-terminated

		ata_ident_cmd_fea_supported* cmd_sets =
			reinterpret_cast<ata_ident_cmd_fea_supported*>(&identify_buf[ATA_IDENT_CMD_SUP_OFFSET]);

		size_t disk_size = 0;

		if (cmd_sets->lba48)
		{
			uint64_t lba48_sectors = *((uint64_t*)&identify_buf[ATA_IDENT_LBA48_TOTAL_SECTORS_OFFSET]);
			disk_size = lba48_sectors * ATA_DEFAULT_SECTOR_SIZE;
		}
		else
		{
			uint64_t lba28_sectors = *((uint64_t*)&identify_buf[ATA_IDENT_LBA28_TOTAL_SECTORS_OFFSET]);
			disk_size = lba28_sectors * ATA_DEFAULT_SECTOR_SIZE;
		}

		// report disk information

		kdebug::kdebug_log("%s Disk: capacity %lld bytes, serial %s, model %s\n",
			atapi ? "ATAPI" : "ATA",
			disk_size,
			serial_num,
			model_num);

	} while (0);

	delete[] identify_buf;

	return ERROR_SUCCESS;
}

error_code ahci::ahci_port_send_command(ahci_port* port,
	uint8_t cmd_id,
	bool atapi,
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
	cl_entry->dw0.atapi = atapi;

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