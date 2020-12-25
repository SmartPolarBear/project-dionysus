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
	size_t old_block_count = roundup(EXT2_INODE_SIZE(inode), block_size) / block_size;

	// TODO: support more indirect block

	auto addr_count = ADDR_COUNT_PER_BLOCK(block_size);

	if (new_block_count > EXT2_DIRECT_BLOCK_COUNT + addr_count ||
		old_block_count > EXT2_DIRECT_BLOCK_COUNT + addr_count)
	{
		return -ERROR_INVALID;
	}

	// shrink
	if (new_block_count < old_block_count)
	{
		for (size_t index = new_block_count; index < old_block_count; index++)
		{
			auto ret = ext2_inode_get_index(fs, inode, index);

			if (has_error(ret))
			{
				return get_error_code(ret);
			}

			auto block = get_result(ret);

			auto err = ext2_block_free(fs, block);
			if (err != ERROR_SUCCESS)
			{
				return err;
			}
		}

		// task L1
		if (old_block_count > EXT2_DIRECT_BLOCK_COUNT
			&& new_block_count <= EXT2_DIRECT_BLOCK_COUNT + addr_count)
		{
			if (inode->indirect_block_l1 == 0)
			{
				return -ERROR_INVALID;
			}

			auto err = ext2_block_free(fs, inode->indirect_block_l1);
			inode->indirect_block_l1 = 0;

			if (err != ERROR_SUCCESS)
			{
				return err;
			}

			// TODO: l2 and l3
		}
	}
	else if (new_block_count >= old_block_count) // expand
	{
		for (auto i = old_block_count; i < new_block_count; i++)
		{
			auto ret = ext2_block_alloc(fs);

			if (has_error(ret))
			{
				return get_error_code(ret);
			}

			auto err = ext2_inode_set_index(fs, inode, i, get_result(ret));
			if (err != ERROR_SUCCESS)
			{
				return err;
			}
		}
	}

	ext2_inode_set_size(inode, new_size);

	auto real_block_count = new_block_count + (new_block_count > EXT2_DIRECT_BLOCK_COUNT);

	inode->sector_count = real_block_count;

	return ERROR_SUCCESS;
}