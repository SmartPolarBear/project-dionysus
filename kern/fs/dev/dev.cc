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

constexpr uintptr_t MBR_BOOTLOADER_OFFSET = 0x000;
constexpr uintptr_t MBR_UNIQUE_ID_OFFSET = 0x1B8;
constexpr uintptr_t MBR_RESERVED_OFFSET = 0x1BC;
constexpr uintptr_t MBR_FIRST_PARTITION_ENTRY_OFFSET = 0x1BE;
constexpr uintptr_t MBR_SECOND_PARTITION_ENTRY_OFFSET = 0x1CE;
constexpr uintptr_t MBR_THIRD_PARTITION_ENTRY_OFFSET = 0x1DE;
constexpr uintptr_t MBR_FOURTH_PARTITION_ENTRY_OFFSET = 0x1EE;
constexpr uintptr_t MBR_VALID_SIGNATURE_OFFSET = 0x1FE;

struct chs_struct
{
	uint32_t head: 8;
	uint32_t sector: 6;
	uint32_t cylinder: 10;
}__attribute__((__packed__));

struct mbr_partition_table_entry
{
	uint8_t bootable;
	chs_struct start_chs;
	uint8_t sys_id;
	chs_struct end_chs;
	uint32_t start_lba;
	uint32_t sector_count;
}__attribute__((__packed__));

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

error_code file_system::device_enumerate_partitions(IDevice& device, VNodeBase* vnode)
{
	if (device.block_size == 2048)
	{
		// This should be a CD-ROM, it can't have partitions
		return ERROR_SUCCESS;
	}

	uint8_t* head_data = new uint8_t[1024];
	constexpr size_t HEAD_DATA_SIZE = sizeof(uint8_t[1024]);
	constexpr uint8_t mbr_sig[] = { 0x55, 0xAA };

	memset(head_data, 0, HEAD_DATA_SIZE);

	error_code ret = ERROR_SUCCESS;
	if ((ret = device.read(head_data, 0, HEAD_DATA_SIZE)) != ERROR_SUCCESS)
	{
		delete[] head_data;
		return ERROR_SUCCESS;
	}

	if (strncmp(reinterpret_cast<const char*>(head_data + 512), "EFI PART", 8) == 0)
	{
		//TODO: support GPT
		return -ERROR_UNSUPPORTED;
	}
	else if (memcmp(head_data + 510, mbr_sig, sizeof(uint8_t[2])) == 0)
	{
		// Parse the master boot record
		// print the *optional* disk signature and reserved field
		char signature[5], reserved[3];
		memset(signature, '\0', sizeof(signature));
		memset(reserved, '\0', sizeof(reserved)); // ensure they are null-terminated

		memmove(signature, head_data + MBR_UNIQUE_ID_OFFSET, sizeof(char[4]));
		memmove(reserved, head_data + MBR_RESERVED_OFFSET, sizeof(char[2]));

		write_format("Unique disk ID: %s, reserved field: %s\n", signature, reserved);

		for (uintptr_t offset : { MBR_FIRST_PARTITION_ENTRY_OFFSET,
								  MBR_SECOND_PARTITION_ENTRY_OFFSET,
								  MBR_THIRD_PARTITION_ENTRY_OFFSET,
								  MBR_FIRST_PARTITION_ENTRY_OFFSET })
		{
			mbr_partition_table_entry* entry =
				reinterpret_cast<mbr_partition_table_entry*>(head_data + offset);

			if (entry->sys_id == 0) // unused entry
			{
				continue;
			}

			write_format("Partition %s, system ID : %d, start LBA: %d, sectors %d\n",
				entry->bootable == 0x80 ? "active" : "inactive",
				entry->sys_id,
				entry->start_lba,
				entry->sector_count
			);
		}
	}

	delete[] head_data;
	return ERROR_SUCCESS;
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

	VNodeBase* node = nullptr;
	if (cls == DEV_CLASS_BLOCK)
	{
		node = new DevFSVNode(VNT_BLK, name);
	}
	else if (cls == DEV_CLASS_CHAR)
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

	ret = device_enumerate_partitions(dev, node);

	return ret;
}