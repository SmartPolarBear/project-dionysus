#include "arch/amd64/cpu/port_io.h"

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

#include "fs/device/ata_devices.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>
#include <fs/vfs/vfs.hpp>

using namespace ahci;

error_code file_system::partition_add_device(file_system::vnode_base& parent,
	logical_block_address lba,
	size_t size,
	size_t disk_idx,
	[[maybe_unused]]uint32_t sys_id)
{
	constexpr size_t PARTITION_NAME_LEN = 32;
	char namebuf[PARTITION_NAME_LEN] = { 0 };

	auto parent_name = parent.get_name();
	auto parent_name_len = strnlen(parent_name, PARTITION_NAME_LEN);

	strncpy(namebuf, parent_name, parent_name_len);
	namebuf[parent_name_len++] = '0' + static_cast<uint8_t>(disk_idx);
	namebuf[parent_name_len++] = '\0';

	auto* dev = new (std::nothrow)file_system::ata_partition_device((ata_block_device*)(parent.dev), lba, size);

	device_add(DC_BLOCK, DBT_PARTITION, *dev, namebuf);

	return ERROR_SUCCESS;
}

error_code file_system::ata_block_device::enumerate_partitions(file_system::vnode_base& parent)
{
	constexpr size_t HEAD_DATA_SIZE = sizeof(uint8_t[1024]);


	// 1024 bytes should contain both MBR and GPT
	uint8_t* head_data = new (std::nothrow)uint8_t[1024];
	memset(head_data, 0, HEAD_DATA_SIZE);

	auto read_ret = this->read(head_data, 0, HEAD_DATA_SIZE);
	if (has_error(read_ret))
	{
		delete[] head_data;
		return get_error_code(read_ret);
	}

	// Compare last two bytes to identify valid MBR disk
	if (memcmp(head_data + 512, GPT_SIG, sizeof(uint8_t[8])) == 0)
	{
		//TODO: handle GPT
		return -ERROR_NOT_IMPL;
	}
	else if (memcmp(head_data + 510, MBR_SIG, sizeof(uint8_t[2])) == 0)
	{
		// Parse the master boot record
		// print the *optional* disk signature and reserved field
		uint64_t signature = 0;
		uint32_t reserved = 0;

		memmove(&signature, head_data + MBR_UNIQUE_ID_OFFSET, sizeof(uint8_t[4]));
		memmove(&reserved, head_data + MBR_RESERVED_OFFSET, sizeof(uint8_t[2]));

		kdebug::kdebug_log("Unique disk ID: 0x%x, reserved field: 0x%x\n", signature, reserved);

		size_t partition_count = 0;

		for (uintptr_t offset : { MBR_FIRST_PARTITION_ENTRY_OFFSET,
								  MBR_SECOND_PARTITION_ENTRY_OFFSET,
								  MBR_THIRD_PARTITION_ENTRY_OFFSET,
								  MBR_FOURTH_PARTITION_ENTRY_OFFSET })
		{
			auto entry = reinterpret_cast<mbr_partition_table_entry*>(head_data + offset);

			if (entry->sys_id == 0) // unused entry
			{
				continue;
			}

			partition_count++;

			kdebug::kdebug_log("Partition %s, system ID : 0x%x, start LBA: 0x%x, sectors %d\n",
				entry->bootable == 0x80 ? "active" : "inactive",
				entry->sys_id,
				entry->start_lba,
				entry->sector_count
			);

			auto err = partition_add_device(parent,
				entry->start_lba,
				entry->sector_count * 512,
				partition_count,
				entry->sys_id);

			if (err != ERROR_SUCCESS)
			{
				delete[] head_data;
				return err;
			}
		}
	}

	delete[] head_data;
	return ERROR_SUCCESS;
}

error_code file_system::ata_block_device::ioctl([[maybe_unused]]size_t req, [[maybe_unused]]void* args)
{
	return -ERROR_UNSUPPORTED;
}

error_code_with_result<size_t> file_system::ata_block_device::write(const void* buf, uintptr_t offset, size_t sz)
{
	if (offset % this->block_size)
	{
		return -ERROR_INVALID;
	}

	if (sz % this->block_size)
	{
		return -ERROR_INVALID;
	}

	ahci_port* port = reinterpret_cast<ahci_port*>(this->dev_data);

	if (port == nullptr)
	{
		return -ERROR_INVALID;
	}

	logical_block_address lba = offset / this->block_size;

	auto ret = ahci_port_send_command(port, ATA_CMD_WRITE_DMA_EX, false, lba, const_cast<void*>(buf), sz);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return sz;
}

error_code_with_result<size_t> file_system::ata_block_device::read(void* buf, uintptr_t offset, size_t sz)
{
	if (offset % this->block_size)
	{
		return -ERROR_INVALID;
	}

	if (sz % this->block_size)
	{
		return -ERROR_INVALID;
	}

	ahci_port* port = reinterpret_cast<ahci_port*>(this->dev_data);

	if (port == nullptr)
	{
		return -ERROR_INVALID;
	}

	logical_block_address lba = offset / this->block_size;

	auto ret = ahci_port_send_command(port, ATA_CMD_READ_DMA_EX, false, lba, buf, sz);

	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return sz;
}

file_system::ata_block_device::~ata_block_device() = default;

file_system::ata_block_device::ata_block_device(ahci::ahci_port* port)
{
	this->dev_data = port;
	this->flags = 0;
	this->block_size = ATA_DEFAULT_SECTOR_SIZE;

	this->features = DFE_HAS_PARTITIONS;
}

error_code file_system::ata_block_device::mmap([[maybe_unused]]uintptr_t base,
	[[maybe_unused]]size_t page_count,
	[[maybe_unused]]int prot,
	[[maybe_unused]]size_t flags)
{
	return -ERROR_UNSUPPORTED;
}

error_code_with_result<size_t> file_system::ata_partition_device::read(void* buf, uintptr_t offset, size_t size)
{
	partition_data* part = (partition_data*)this->dev_data;
	size_t part_size_bytes = part->size * 512;
	size_t part_off_bytes = part->lba_start * 512;

	if (offset >= part_size_bytes)
	{
		return -1;
	}

	size_t len = std::min(part_size_bytes - offset, size);

	auto ret = part->parent_dev->read(buf, offset + part_off_bytes, len);

	return ret;
}

error_code_with_result<size_t> file_system::ata_partition_device::write(const void* buf, uintptr_t offset, size_t size)
{
	partition_data* part = (partition_data*)this->dev_data;
	size_t part_size_bytes = part->size * 512;
	size_t part_off_bytes = part->lba_start * 512;

	if (offset >= part_size_bytes)
	{
		return -1;
	}

	size_t len = std::min(part_size_bytes - offset, size);

	auto ret = part->parent_dev->write(buf, offset + part_off_bytes, len);

	return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

error_code file_system::ata_partition_device::ioctl(size_t req, void* args)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::ata_partition_device::mmap(uintptr_t base, size_t page_count, int prot, size_t flags)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::ata_partition_device::enumerate_partitions(file_system::vnode_base& parent)
{
	return -ERROR_UNSUPPORTED;
}

#pragma GCC diagnostic pop

file_system::ata_partition_device::ata_partition_device(file_system::ata_block_device* parent,
	logical_block_address lba,
	size_t sz)
{
	partition_data* data = new (std::nothrow)partition_data;

	data->parent_dev = parent;
	data->lba_start = lba;
	data->size = sz;

	this->dev_data = data;
	this->flags = 0;
	this->block_size = ATA_DEFAULT_SECTOR_SIZE;

	this->features = 0;
}
