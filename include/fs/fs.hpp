#pragma once
#include "system/types.h"

#include "debug/nullability.hpp"

#include "fs/mbr.hpp"
#include "fs/device/device.hpp"
#include "fs/vfs/vfs.hpp"

#include <cstring>

namespace file_system
{
	using fs_find_pred = bool (*)(const fs_class_base*);


	PANIC void fs_init();
	error_code_with_result<fs_instance*> fs_create(fs_class_base* fs_class,
		device_class* dev,
		size_t flags,
		const char* data);

	error_code fs_register(fs_class_base* fs_class);

	[[maybe_unused]]fs_class_base* fs_find(fs_class_id id);
	[[maybe_unused]]fs_class_base* fs_find(const char* name);

}