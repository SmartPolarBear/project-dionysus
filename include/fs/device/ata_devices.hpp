#pragma once

#include "fs/device/dev.hpp"
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

	class ATABlockDevice :
		public file_system::device_class
	{
	 public:
		explicit ATABlockDevice(ahci::ahci_port* port);

		~ATABlockDevice() override;

		size_t read(void* buf, uintptr_t offset, size_t count) override;
		size_t write(const void* buf, uintptr_t offset, size_t count) override;
		error_code ioctl(size_t req, void* args) override;
		error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) override;
		error_code enumerate_partitions(vnode_base& parent) override;
	};

	class ATAPartitionDevice :
		public file_system::device_class
	{
	 public:
		explicit ATAPartitionDevice(ATABlockDevice& parent, logical_block_address lba, size_t sz);

		~ATAPartitionDevice() override = default;
		size_t read(void* buf, uintptr_t offset, size_t count) override;
		size_t write(const void* buf, uintptr_t offset, size_t count) override;
		error_code ioctl(size_t req, void* args) override;
		error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) override;
		error_code enumerate_partitions(vnode_base& parent) override;
	};
}