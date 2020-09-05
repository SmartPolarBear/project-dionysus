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

VNodeBase* devfs_root = nullptr;

constexpr mode_type DEVICE_DEFAULT_MODE = 0600;

error_code file_system::devfs_create_root_if_not_exist()
{
	devfs_root = static_cast<VNodeBase*>(new DevFSVNode(VNT_BLK, nullptr));
	if (!devfs_root)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	devfs_root->flags |= VNF_MEMORY;
	devfs_root->mode = 0555;
	devfs_root->uid = 0;
	devfs_root->gid = 0;

	return ERROR_SUCCESS;
}

static inline error_code device_new_name(dev_class cls, size_t sbcls, OUT char* namebuf)
{
	if (namebuf == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (cls == DC_BLOCK)
	{
		switch (sbcls)
		{
		case DBT_CDx:
			if (block_dev_cd_idx_char > 'z')
			{
				return -ERROR_UNSUPPORTED;
			}
			strncpy(namebuf, block_dev_cd_name, 3);
			namebuf[3] = '\0';// ensure null-terminated

			namebuf[2] = block_dev_cd_idx_char++;

			break;

		case DBT_SDx:
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
		if ((ret = device_new_name(cls, subcls, node_name)) != ERROR_SUCCESS)
		{
			delete[] node_name;
			return ret;
		}

		name = node_name;
	}

	VNodeBase* node = nullptr;
	if (cls == DC_BLOCK)
	{
		node = new DevFSVNode(VNT_BLK, name);
	}
	else if (cls == DC_CHAR)
	{
		node = new DevFSVNode(VNT_CHR, name);
	}
	else
	{
		delete[] node_name;
		return -ERROR_INVALID;
	}

	// we have copied its content, so release the memory
	delete[] node_name;

	node->dev = &dev;

	// Use inode number to store full device class:subclass
	node->ino = ((uint32_t)cls) | ((uint64_t)subcls << 32u);

	node->flags |= VNF_MEMORY;

	// Default permissions for devices
	node->mode = DEVICE_DEFAULT_MODE;
	node->uid = 0;
	node->gid = 0;

	devfs_create_root_if_not_exist();

	libkernel::list_add(&node->link, &devfs_root->child_head);

	if (dev.features & DFE_HAS_PARTITIONS)
	{
		dev.enumerate_partitions(*node);
	}

	return ret;
}