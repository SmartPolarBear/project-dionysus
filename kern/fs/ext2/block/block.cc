#include "../include/block.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "debug/kdebug.h"

using namespace file_system;

error_code ext2_block_read(file_system::fs_instance* fs, uint8_t* buf, size_t block_num)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ret = fs->dev->read(buf, block_num * ext2data->get_block_size(), ext2data->get_block_size());
	if (has_error(ret))
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

	const size_t block_size = ext2data->get_block_size();

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ret = fs->dev->write(buf, block_num * block_size, block_size);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	if (get_result(ret) != block_size)
	{
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}

// TODO:Support more than single-block block bitmap
error_code_with_result<uint64_t> ext2_block_alloc(file_system::fs_instance* fs)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);
	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto superblock = ext2data->get_superblock();

	uint64_t* bitmap_buf = new(std::nothrow)uint64_t[ext2data->get_block_size() / sizeof(uint64_t)];
	if (bitmap_buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	for (size_t i = 0; i < ext2data->get_bgdt_entry_count(); i++)
	{
		auto bgd = ext2data->get_bgd_by_index(i);
		if (!bgd.free_block_count)
		{
			continue;
		}

		if (auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(bitmap_buf), bgd.block_bitmap_no);ret
			!= ERROR_SUCCESS)
		{
			delete[] bitmap_buf;
			return ret;
		}

		for (size_t j = 0; j < superblock.block_group_block_count; j++)
		{
			// enumerate blocks
			if (!(bitmap_buf[j / 64ull] & (1ull << (j % 64ull))))
			{

				// write back the bitmap block
				auto bgd = ext2data->get_bgd_by_index(i);
				bitmap_buf[j / 64] |= (1ull << (j % 64));

				if (auto ret = ext2_block_write(fs, reinterpret_cast<const uint8_t*>(bitmap_buf), bgd.block_bitmap_no);
					ret != ERROR_SUCCESS)
				{
					break;
				}

				bgd.free_block_count--;
				auto bdg_block_count = (i * sizeof(ext2_block_group_desc)) / ext2data->get_block_size();


				if (auto ret = ext2_block_write(fs,
						reinterpret_cast<uint8_t*>(ext2data->get_bgdt()) + bdg_block_count * ext2data->get_block_size(),
						bdg_block_count + 2);
					ret != ERROR_SUCCESS)
				{
					break;
				}

				superblock.free_block_count--;
				ext2data->superblock_write_back(fs);

				delete[] bitmap_buf;
				return i * superblock.block_group_block_count + j + 1; // block

			}
		}
	}

	delete[] bitmap_buf;
	return -ERROR_NO_ENTRY;
}

error_code ext2_block_free(file_system::fs_instance* fs, uint32_t block)
{
	if (block == 0)
	{
		return -ERROR_INVALID;
	}

	if (fs == nullptr)
	{
		return -ERROR_INVALID;
	}

	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto superblock = ext2data->get_superblock();

	uint64_t* bitmap_buf = new(std::nothrow)uint64_t[ext2data->get_block_size() / sizeof(uint64_t)];

	auto index = EXT2_BLOCK_INDEX_IN_BLOCK_GROUP(block, superblock.block_group_block_count);
	auto group = EXT2_BLOCK_GET_BLOCK_GROUP(block, superblock.block_group_block_count);

	auto bgd = ext2data->get_bgdt() + group;

	// read the bitmap
	auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(bitmap_buf), bgd->block_bitmap_no);
	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	bitmap_buf[index / 64] &= ~(1ULL << (index % 64ull)); // unset the bit

	// write back the bitmap
	ret = ext2_block_write(fs, reinterpret_cast<uint8_t*>(bitmap_buf), bgd->block_bitmap_no);
	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	bgd->free_block_count++;
	auto bdg_block_count = (group * sizeof(ext2_block_group_desc)) / ext2data->get_block_size();
	ret = ext2_block_write(fs,
		reinterpret_cast<uint8_t*>(ext2data->get_bgdt()) + bdg_block_count * ext2data->get_block_size(),
		bgd->block_bitmap_no + 2);

	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	superblock.free_inode_count++;
	ret = ext2data->superblock_write_back(fs);

	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	delete[] bitmap_buf;
	return ERROR_SUCCESS;
}


