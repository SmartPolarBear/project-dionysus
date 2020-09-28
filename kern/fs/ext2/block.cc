#include "include/block.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;

error_code ext2_block_read(file_system::fs_instance* fs, uint8_t* buf, size_t block_num)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ret = fs->dev->read(buf, block_num * ext2data->get_block_size(), ext2data->get_block_size());
	if (get_error_code(ret) != ERROR_SUCCESS)
	{
		return get_error_code(ret);
	}

	if (get_result(ret) != ext2data->get_block_size())
	{
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}

error_code ext2_block_write(file_system::fs_instance* fs, const uint8_t* buf, size_t block_num)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ret = fs->dev->write(buf, block_num * ext2data->get_block_size(), ext2data->get_block_size());
	if (get_error_code(ret) != ERROR_SUCCESS)
	{
		return get_error_code(ret);
	}

	if (get_result(ret) != ext2data->get_block_size())
	{
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}

error_code_with_result<uint64_t> ext2_block_alloc(file_system::fs_instance* fs)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	uint64_t* bitmap_buf = new uint64_t[ext2data->get_block_size() / sizeof(uint64_t)];
	auto superblock = ext2data->get_superblock();

	error_code ret = ERROR_SUCCESS;

	for (size_t i = 0; i < ext2data->get_bgdt_entry_count(); i++)
	{
		auto bgd = ext2data->get_bgd_by_index(i);
		if (!bgd.free_blocks)
		{
			continue;
		}

		ext2_block_read(fs, reinterpret_cast<uint8_t*>(bitmap_buf), bgd.block_bitmap_no);

		for (size_t j = 0; j < superblock.block_group_blocks; j++)
		{
			// enumerate blocks
			if (!(bitmap_buf[j / 64ull] & (1ull << (j % 64ull))))
			{

				// write back the bitmap block
				auto bgd = ext2data->get_bgd_by_index(i);
				bitmap_buf[j / 64] |= (1ull << (j % 64));

				if ((ret = ext2_block_write(fs, reinterpret_cast<const uint8_t*>(bitmap_buf), bgd.block_bitmap_no))
					!= ERROR_SUCCESS)
				{
					break;
				}

				bgd.free_blocks--;
				auto bdg_block_count = (i * sizeof(ext2_blkgrp_desc)) / ext2data->get_block_size();

				if ((ret = ext2_block_write(fs,
					reinterpret_cast<uint8_t*>(ext2data->get_bgdt()) + bdg_block_count * ext2data->get_block_size(),
					bgd.block_bitmap_no))
					!= ERROR_SUCCESS)
				{
					break;
				}

				superblock.free_blocks--;
				ext2data->superblock_write_back(fs);

				delete[] bitmap_buf;

				return i * superblock.block_group_blocks + j + 1; // block

			}
		}
	}

	delete[] bitmap_buf;
	return ret;
}

error_code ext2_block_free(file_system::fs_instance* fs, uint32_t block)
{
	return 0;
}


