#include "include/inode.hpp"
#include "include/block.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

#include <cstring>

using namespace file_system;

error_code ext2_inode_read(file_system::fs_instance* fs, ext2_ino_type inum, OUT ext2_inode* inode)
{

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);
	auto superblock = data->get_superblock();

	if (inum < EXT2_FIRST_INODE_NUMBER || inum >= superblock.inode_count)
	{
		return -ERROR_INVALID;
	}

	auto inode_group = EXT2_INODE_GET_BLOCK_GROUP(inum, superblock.block_group_inodes);
	auto inode_group_index = EXT2_INODE_INDEX_IN_BLOCK_GROUP(inum, superblock.block_group_inodes);

	auto inode_block =
		data->get_bgd_by_index(inode_group).inode_table_no + inode_group_index / data->get_inodes_per_block();

	auto offset_in_block = (inode_group_index % data->get_inodes_per_block()) * data->get_inode_size();

	if (offset_in_block >= data->get_block_size())
	{
		return -ERROR_INVALID;
	}

	uint8_t* block_buf = new uint8_t[data->get_block_size()];

	auto ret = ext2_block_read(fs, block_buf, inode_block);

	if (ret != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return ret;
	}

	memmove(inode, block_buf + offset_in_block, data->get_inode_size());

	return ERROR_SUCCESS;
}

error_code ext2_inode_write(file_system::fs_instance* fs,
	file_system::ext2_ino_type inum,
	IN const file_system::ext2_inode* inode)
{
	//TODO: update the modified time of inode

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);
	auto superblock = data->get_superblock();

	if (inum < EXT2_FIRST_INODE_NUMBER || inum >= superblock.inode_count)
	{
		return -ERROR_INVALID;
	}

	auto inode_group = EXT2_INODE_GET_BLOCK_GROUP(inum, superblock.block_group_inodes);
	auto inode_group_index = EXT2_INODE_INDEX_IN_BLOCK_GROUP(inum, superblock.block_group_inodes);

	auto inode_block =
		data->get_bgd_by_index(inode_group).inode_table_no + inode_group_index / data->get_inodes_per_block();

	auto offset_in_block = (inode_group_index % data->get_inodes_per_block()) * data->get_inode_size();

	if (offset_in_block >= data->get_block_size())
	{
		return -ERROR_INVALID;
	}

	uint8_t* block_buf = new uint8_t[data->get_block_size()];

	auto ret = ext2_block_read(fs, block_buf, inode_block);

	if (ret != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return ret;
	}

	memmove(block_buf + offset_in_block, inode, data->get_inode_size());

	return ext2_block_write(fs, block_buf, inode_block);
}
