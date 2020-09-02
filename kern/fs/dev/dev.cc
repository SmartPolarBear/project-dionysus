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

#include "fs/device/ATABlockDevice.hpp"
#include "fs/device/dev.hpp"
#include "fs/vfs/vnode.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

using namespace file_system;

[[maybe_unused]] char block_dev_sd_idx_char = 'a';
[[maybe_unused]] char block_dev_cd_idx_char = 'a';
[[maybe_unused]] char block_dev_hd_idx_char = 'a';

[[maybe_unused]]const char* block_dev_sd_name = "sd ";
[[maybe_unused]]const char* block_dev_cd_name = "cd ";
[[maybe_unused]]const char* block_dev_hd_name = "hd ";

static inline error_code device_new_name(dev_class cls, size_t sbcls, OUT char* namebuf)
{
	if (namebuf == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (cls == DEV_CLASS_BLOCK)
	{
		switch (sbcls)
		{
		case DEV_BLOCK_CDx:
			if (block_dev_cd_idx_char > 'z')
			{
				return -ERROR_UNSUPPORTED;
			}
			strncpy(namebuf, block_dev_cd_name, 3);
			namebuf[3] = '\0';// ensure null-terminated

			namebuf[2] = block_dev_cd_idx_char++;

			break;

		case DEV_BLOCK_SDx:
			if (block_dev_sd_idx_char > 'z')
			{
				return -ERROR_UNSUPPORTED;
			}
			strncpy(namebuf, block_dev_hd_name, 3);
			namebuf[3] = '\0';// ensure null-terminated

			namebuf[2] = block_dev_hd_idx_char++;
			break;

		default:
			return -ERROR_INVALID;
		}

		return ERROR_SUCCESS;
	}
	return -ERROR_INVALID;
}

error_code file_system::device_add(dev_class cls, size_t subcls, IDevice& dev, const char* name)
{
	char* node_name = new char[64];
	error_code ret = ERROR_SUCCESS;

	if (!name)
	{
		if ((ret == device_new_name(cls, subcls, node_name)) != ERROR_SUCCESS)
		{
			delete[] node_name;
			return ret;
		}

		name = node_name;
	}

	IVNode* node = nullptr;
	if (cls == DEV_CLASS_BLOCK)
	{
		node = new DEVFSVNode(VNT_BLK, name);
	}
	else if (cls == DEV_CLASS_CHAR)
	{
		node = new DEVFSVNode(VNT_CHR, name);
	}
	else
	{
		delete[] node_name;
		return -ERROR_INVALID;
	}

	delete[] node_name;
	return ERROR_SUCCESS;
}