#pragma once
#include "system/types.h"

#include "fs/mbr.hpp"
#include "fs/device/dev.hpp"
#include "fs/vfs/vfs.hpp"

#include <cstring>

namespace file_system
{
	using fs_find_pred = bool (*)(const fs_class_base*);


	PANIC void fs_init();
	error_code fs_create(fs_class_base* fs_class, device_class* dev, size_t flags, void* data);
	error_code fs_register(fs_class_base* fs_class);

	fs_class_base* fs_find(fs_find_pred pred);
	fs_class_base* fs_find(fs_class_id id);
	fs_class_base* fs_find(const char* name);

}