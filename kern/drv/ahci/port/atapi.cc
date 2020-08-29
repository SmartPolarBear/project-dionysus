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

error_code ahci::atapi_identify_device(ahci_port* port)
{
	uint16_t* identify_buf = new uint16_t[256];

	char model_num[40] = {};
	char serial_num[20] = {};

	error_code ret = ERROR_SUCCESS;

	do
	{
		if ((ret = ahci_port_send_command(port, ATA_CMD_PACKET_IDENTIFY, 0, identify_buf, sizeof(uint16_t[256])))
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

		kdebug::kdebug_log("ATAPI Disk: capacity %lld bytes, serial %s, model %s\n",
			disk_size,
			serial_num,
			model_num);

	} while (0);

	delete[] identify_buf;

	return ERROR_SUCCESS;
}

