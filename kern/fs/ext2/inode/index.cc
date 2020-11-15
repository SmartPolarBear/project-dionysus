#include "../include/inode.hpp"
#include "../include/block.hpp"

#include "drivers/cmos/rtc.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

#include <cstring>

using namespace file_system;

error_code_with_result<uint32_t> ext2_inode_get_index(file_system::fs_instance* fs,
	file_system::ext2_inode* inode,
	uint32_t index)
{
	ext2_data* data = (ext2_data*)fs->private_data;

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (index < EXT2_DIRECT_BLOCK_COUNT)
	{
		return inode->direct_blocks[index];
	}

	auto addr_count = ADDR_COUNT_PER_BLOCK(data->get_block_size());

	block_address_type* addrs = new block_address_type[addr_count];

	if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count)
	{
		if (inode->indirect_block_l1 == 0)
		{
			delete[] addrs;
			return -ERROR_OUT_OF_BOUND;
		}

		auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(addrs), inode->indirect_block_l1);
		if (ret != ERROR_SUCCESS)
		{
			delete[] addrs;
			return ret;
		}

		delete[] addrs;
		return addrs[index - EXT2_DIRECT_BLOCK_COUNT];
	}
	else if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count * 2)
	{
		delete[] addrs;
		return -ERROR_NOT_IMPL;// TODO:L2
	}
	else if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count * 3)
	{
		delete[] addrs;
		return -ERROR_NOT_IMPL; // TODO:L3
	}
	else
	{
		delete[] addrs;
		return -ERROR_INVALID;
	}

	delete[] addrs;
	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code ext2_inode_set_index(file_system::fs_instance* fs,
	file_system::ext2_inode* inode,
	uint32_t index,
	uint32_t value)
{
	ext2_data* data = (ext2_data*)fs->private_data;

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto addr_count = ADDR_COUNT_PER_BLOCK(data->get_block_size());
	block_address_type* addrs = new block_address_type[addr_count];

	if (index < EXT2_DIRECT_BLOCK_COUNT)
	{
		delete[] addrs;
		inode->direct_blocks[index] = value;
		return ERROR_SUCCESS;
	}
	else if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count)
	{
		if (inode->indirect_block_l1 == 0)
		{
			auto ret = ext2_block_alloc(fs);
			if (has_error(ret))
			{
				delete[]addrs;
				return get_error_code(ret);
			}

			block_device_type block = static_cast<block_device_type>(get_result(ret));

			inode->indirect_block_l1 = block;
		}

		auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(addrs), inode->indirect_block_l1);
		if (ret != ERROR_SUCCESS)
		{
			delete[] addrs;
			return ret;
		}

		addrs[index - EXT2_DIRECT_BLOCK_COUNT] = value;

		ret = ext2_block_write(fs, reinterpret_cast<uint8_t*>(addrs), inode->indirect_block_l1);
		if (ret != ERROR_SUCCESS)
		{
			delete[] addrs;
			return ret;
		}

		delete[] addrs;
		return ERROR_SUCCESS;
	}
	else if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count * 2)
	{
		delete[] addrs;
		return -ERROR_NOT_IMPL;// TODO:L2
	}
	else if (index < EXT2_DIRECT_BLOCK_COUNT + addr_count * 3)
	{
		delete[] addrs;
		return -ERROR_NOT_IMPL; // TODO:L3
	}
	else
	{
		delete[] addrs;
		return -ERROR_INVALID;
	}

	delete[] addrs;
	return -ERROR_INVALID;
}

