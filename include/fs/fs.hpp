#pragma once
#include "system/types.h"

#include "fs/mbr.hpp"
#include "fs/device/dev.hpp"
#include "fs/vfs/vfs.hpp"

#include <cstring>

namespace file_system
{
	PANIC void fs_init();
	error_code fs_create(fs_class_base* fs_class, device_class* dev, size_t flags, void* data);
}