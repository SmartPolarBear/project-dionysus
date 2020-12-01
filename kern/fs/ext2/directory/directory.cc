#include "../include/directory.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

error_code ext2_directory_inode_insert(file_system::fs_instance* fs,
	file_system::ext2_ino_type at_ino,
	file_system::ext2_inode* at_inode,
	const char* name,
	file_system::ext2_ino_type ino,
	file_system::vnode_types type)
{
	return 0;
}

error_code ext2_directory_inode_remove(file_system::fs_instance* fs,
	file_system::ext2_ino_type at_ino,
	file_system::ext2_inode* at_inode,
	file_system::vnode_base* vn)
{
	return 0;
}
