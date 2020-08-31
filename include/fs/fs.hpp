#pragma once
#include "system/types.h"

namespace file_system
{
	class block_device
	{
	 protected:
		void* dev_data;
		size_t block_size;
		size_t flags;
	 public:
		virtual ~block_device()
		{
		};

		virtual size_t read(void* buf, uintptr_t offset, size_t count) = 0;
		virtual size_t write(const void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code ioctl(size_t req, void* args) = 0;
		virtual error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) = 0;
	};

	PANIC void fs_init();
}