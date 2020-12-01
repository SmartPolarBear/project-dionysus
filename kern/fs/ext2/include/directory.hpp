#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

[[nodiscard]]error_code ext2_directory_inode_insert(file_system::fs_instance* fs,
	file_system::ext2_ino_type at_ino,
	IN file_system::ext2_inode* at_inode,
	const char* name,
	file_system::ext2_ino_type ino,
	file_system::vnode_types type
);
[[nodiscard]]error_code ext2_directory_inode_remove(file_system::fs_instance* fs,
	file_system::ext2_ino_type at_ino,
	IN file_system::ext2_inode* at_inode,
	IN file_system::vnode_base* vn);