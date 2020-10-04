#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include <cstring>

namespace file_system
{
	constexpr mode_type DEVICE_DEFAULT_MODE = 0600;
	constexpr mode_type ROOT_DEFAULT_MODE = 0555;

	class vnode_base;

	enum device_class_id
	{
		DC_BLOCK = 1,
		DC_CHAR = 2,
		DC_ANY = 255
	};

	class device_class
	{
	 protected:
		void* dev_data;
		size_t block_size;
		size_t flags;
		size_t features;
	 public:
		virtual ~device_class() = default;

		virtual error_code_with_result<size_t> read(void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code_with_result<size_t> write(const void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code ioctl(size_t req, void* args) = 0;
		virtual error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) = 0;
		virtual error_code enumerate_partitions(vnode_base& parent) = 0;

	 public:
		friend error_code device_add(device_class_id cls, size_t subcls, device_class& dev, const char* name);
	};

	error_code init_devfs_root();

	error_code device_add(device_class_id cls, size_t subcls, IN device_class& dev, NULLABLE const char* name);

}