#pragma once

#include "system/types.h"
#include <cstring>

namespace file_system
{

	class IDevice
	{
	 protected:
		void* dev_data;
		size_t block_size;
		size_t flags;
	 public:
		virtual ~IDevice()
		{
		};

		virtual size_t read(void* buf, uintptr_t offset, size_t count) = 0;
		virtual size_t write(const void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code ioctl(size_t req, void* args) = 0;
	};

	class IMemmap
	{
	 public:
		virtual ~IMemmap()
		{
		};
		virtual error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) = 0;
	};

	enum dev_class
	{
		DEV_CLASS_BLOCK = 1,
		DEV_CLASS_CHAR = 2,
		DEV_CLASS_ANY = 255
	};

	enum block_device_type
	{
		DEV_BLOCK_SDx = 1,
		DEV_BLOCK_HDx = 2,
		DEV_BLOCK_RAM = 3,
		DEV_BLOCK_CDx = 4,
		DEV_BLOCK_PART = 127,
		DEV_BLOCK_PSEUDO = 128,
		DEV_BLOCK_OTHER = 255,
	};

	enum char_device_type
	{
		DEV_CHAR_TTY = 1
	};

	error_code device_add(dev_class cls, size_t subcls, IN IDevice& dev, NULLABLE const char* name);
}