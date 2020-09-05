#pragma once

#include "system/types.h"
#include <cstring>
#include <fs/vfs/vfs.hpp>

namespace file_system
{

	class IDevice
	{
	 protected:
		void* dev_data;
		size_t block_size;
		size_t flags;
		size_t features;
	 public:
		virtual ~IDevice() = default;

		virtual size_t read(void* buf, uintptr_t offset, size_t count) = 0;
		virtual size_t write(const void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code ioctl(size_t req, void* args) = 0;
		virtual error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) = 0;
		virtual error_code enumerate_partitions(VNodeBase& parent) = 0;

	 public:
		friend error_code device_add(dev_class cls, size_t subcls, IDevice& dev, const char* name);
	};

	error_code device_add(dev_class cls, size_t subcls, IN IDevice& dev, NULLABLE const char* name);

}