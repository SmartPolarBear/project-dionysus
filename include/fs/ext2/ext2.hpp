#pragma once

#include "system/types.h"
#include "fs/vfs/vfs.hpp"

namespace file_system
{
	class ext2_fs_class
		: public fs_class_base
	{
	 public:
		vnode_base* get_root(fs_instance* fs) override;
		error_code initialize(fs_instance* fs, const char* data) override;
		void destroy(fs_instance* fs) override;
		error_code statvfs(fs_instance* fs, vfs_status* ret) override;
	};

	extern ext2_fs_class g_ext2fs;
}