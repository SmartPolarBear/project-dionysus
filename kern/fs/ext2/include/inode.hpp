#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

error_code ext2_inode_read(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	OUT file_system::ext2_inode* inode);

error_code ext2_inode_write(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	IN file_system::ext2_inode* inode);