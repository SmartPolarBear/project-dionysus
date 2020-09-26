#include "include/inode.hpp"
#include "include/block.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;

error_code ext2_inode_read(file_system::fs_instance* fs, ext2_ino_type inum, OUT ext2_inode* inode)
{

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);
	auto superblock = data->get_superblock();

	if (inum < EXT2_FIRST_INODE_NUMBER || inum >= superblock.inode_count)
	{
		return -ERROR_INVALID;
	}



	return ERROR_SUCCESS;
}

