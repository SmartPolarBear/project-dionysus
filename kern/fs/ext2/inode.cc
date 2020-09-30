#include "include/inode.hpp"
#include "include/block.hpp"

#include "drivers/cmos/rtc.hpp"

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

	auto inode_group = EXT2_INODE_GET_BLOCK_GROUP(inum, superblock.block_group_inode_count);
	auto inode_group_index = EXT2_INODE_INDEX_IN_BLOCK_GROUP(inum, superblock.block_group_inode_count);

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
	IN  file_system::ext2_inode* inode)
{

	inode->atime = cmos::cmos_read_rtc_timestamp();

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);
	auto superblock = data->get_superblock();

	if (inum < EXT2_FIRST_INODE_NUMBER || inum >= superblock.inode_count)
	{
		return -ERROR_INVALID;
	}

	auto inode_group = EXT2_INODE_GET_BLOCK_GROUP(inum, superblock.block_group_inode_count);
	auto inode_group_index = EXT2_INODE_INDEX_IN_BLOCK_GROUP(inum, superblock.block_group_inode_count);

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

error_code_with_result<uint32_t> ext2_inode_get_index(file_system::fs_instance* fs,
	file_system::ext2_inode* inode,
	uint32_t index)
{
	return ERROR_SUCCESS;
}

error_code ext2_inode_set_index(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	file_system::ext2_inode* inode,
	uint32_t index,
	uint32_t value)
{
	return 0;
}

error_code_with_result<uint32_t> ext2_inode_alloc(file_system::fs_instance* fs, bool is_dir)
{

	return ERROR_SUCCESS;
}

error_code ext2_inode_free(file_system::fs_instance* fs, file_system::ext2_ino_type ino, bool is_dir)
{
	return ERROR_SUCCESS;
}

error_code ext2_inode_resize(file_system::fs_instance* fs,
	file_system::ext2_ino_type ino,
	file_system::ext2_inode* inode,
	size_t new_size)
{
	return ERROR_SUCCESS;
}
