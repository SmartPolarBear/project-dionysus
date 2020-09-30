#include "../include/inode.hpp"
#include "../include/block.hpp"

#include "drivers/cmos/rtc.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

#include <cstring>

using namespace file_system;

error_code ext2_inode_resize(file_system::fs_instance* fs,
	file_system::ext2_ino_type ino,
	file_system::ext2_inode* inode,
	size_t new_size)
{
	auto data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto block_size = data->get_block_size();

	size_t new_block_count = roundup(new_size, block_size) / block_size;
	size_t old_block_count = roundup(ext2_inode_get_size(inode), block_size) / block_size;

	// TODO: support more indirect block

	if (new_block_count > EXT2_DIRECT_BLOCK_COUNT + (block_size / sizeof(uint32_t)) ||
		old_block_count > EXT2_DIRECT_BLOCK_COUNT + (block_size / sizeof(uint32_t)))
	{
		return -ERROR_INVALID;
	}

	// shrink
	if (new_block_count < old_block_count)
	{

	}
	else if (new_block_count >= old_block_count) // expand
	{
		for (auto i = old_block_count; i < new_block_count; i++)
		{
			auto ret = ext2_block_alloc(fs);

			if (get_error_code(ret) != ERROR_SUCCESS)
			{
				return get_error_code(ret);
			}

			auto err = ext2_inode_set_index(fs, ino, inode, i, get_result(ret));
			if (err != ERROR_SUCCESS)
			{
				return err;
			}
		}
	}

	return ERROR_SUCCESS;
}