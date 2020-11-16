#pragma once

#include "fs/device/device.hpp"
#include "system/types.h"

#include "fs/fs.hpp"

#include "drivers/pci/pci_device.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/identify_device.hpp"

namespace file_system
{

	struct partition_data
	{
		device_class* parent_dev;
		uint64_t lba_start;
		uint64_t size;
	};

	class ata_block_device :
		public file_system::device_class
	{
	 public:
		static constexpr uint8_t MBR_SIG[] = { 0x55, 0xAA };
		static constexpr uint8_t GPT_SIG[] = { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T' };
	 public:
		explicit ata_block_device(ahci::ahci_port* port);

		~ata_block_device() override;

		error_code_with_result<size_t> read(void* buf, uintptr_t offset, size_t count) override;
		error_code_with_result<size_t> write(const void* buf, uintptr_t offset, size_t count) override;
		error_code ioctl(size_t req, void* args) override;
		error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) override;
		error_code enumerate_partitions(vnode_base& parent) override;
	};

	class ata_partition_device :
		public file_system::device_class
	{
	 public:
		explicit ata_partition_device(ata_block_device* parent, logical_block_address lba, size_t sz);

		~ata_partition_device() override = default;
		error_code_with_result<size_t> read(void* buf, uintptr_t offset, size_t size) override;
		error_code_with_result<size_t> write(const void* buf, uintptr_t offset, size_t size) override;
		error_code ioctl(size_t req, void* args) override;
		error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) override;
		error_code enumerate_partitions(vnode_base& parent) override;
	};
}