#include "../include/inode.hpp"
#include "../include/block.hpp"

#include "drivers/cmos/rtc.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "debug/kdebug.h"

#include <cstring>

using namespace file_system;

error_code ext2_inode_read(file_system::fs_instance* fs, ext2_ino_type inum, OUT ext2_inode* inode)
{

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

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

	uint8_t* block_buf = new(std::nothrow)uint8_t[data->get_block_size()];

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

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	const size_t block_size = data->get_block_size();
	const size_t inode_size = data->get_inode_size();
	const size_t inodes_per_block = data->get_inodes_per_block();

	auto superblock = data->get_superblock();

	if (inum < EXT2_FIRST_INODE_NUMBER || inum >= superblock.inode_count)
	{
		return -ERROR_INVALID;
	}

	auto inode_group = EXT2_INODE_GET_BLOCK_GROUP(inum, superblock.block_group_inode_count);
	auto inode_index_in_group = EXT2_INODE_INDEX_IN_BLOCK_GROUP(inum, superblock.block_group_inode_count);

	auto inode_block =
		data->get_bgd_by_index(inode_group).inode_table_no + inode_index_in_group / inodes_per_block;

	auto offset_in_block = (inode_index_in_group % inodes_per_block) * inode_size;

	if (offset_in_block >= block_size)
	{
		return -ERROR_INVALID;
	}

	uint8_t* block_buf = new(std::nothrow)uint8_t[block_size];
	if (block_buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	if (auto ret = ext2_block_read(fs, block_buf, inode_block);ret != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return ret;
	}

	memmove(offset_in_block + block_buf, inode, inode_size);

	if (auto ret = ext2_block_write(fs, block_buf, inode_block);ret != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return ret;

	}

	delete[] block_buf;
	return ERROR_SUCCESS;
}

error_code_with_result<uint32_t> ext2_inode_alloc(file_system::fs_instance* fs, bool is_dir)
{
	auto data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	uint64_t* bitmap_buf = new(std::nothrow)uint64_t[data->get_block_size() / sizeof(uint64_t)];
	uint8_t* bitmap_bytes = reinterpret_cast<uint8_t*>(bitmap_buf);
	if (bitmap_buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	const size_t bdgt_entry_count = data->get_bgdt_entry_count();
	ext2_superblock& superblock = data->get_superblock();

	for (size_t i = 0; i < bdgt_entry_count; i++)
	{
		auto bgd = &data->get_bgdt()[i];

		if (bgd->free_inode_count == 0)
		{
			continue;
		}

		if (auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(bitmap_bytes), bgd->block_bitmap_no);ret
			!= ERROR_SUCCESS)
		{
			delete[] bitmap_buf;
			return ret;
		}

		for (size_t j = 0; j < superblock.block_group_inode_count; j++)
		{
			if ((bitmap_buf[j / 64] & (1ull << (j % 64))) == 0) //free
			{
				ext2_ino_type ino = EXT2_FIRST_INODE_NUMBER + i * superblock.block_group_inode_count + j;

				bitmap_buf[j / 64] |= (1ull << (j % 64));

				if (auto ret = ext2_block_write(fs, bitmap_bytes, bgd->block_bitmap_no);
					ret != ERROR_SUCCESS)
				{
					delete[] bitmap_buf;
					return ret;
				}

				bgd->free_inode_count--;
				bgd->directory_count += is_dir;

				size_t bgd_block_num = (i * sizeof(ext2_block_group_desc)) / data->get_block_size();

				if (auto ret = ext2_block_write(fs,
						reinterpret_cast<const uint8_t*>(data->get_bgdt()) + bgd_block_num * data->get_block_size(),
						bgd_block_num + 2);ret != ERROR_SUCCESS)
				{
					delete[] bitmap_buf;
					return ret;
				}

				superblock.free_inode_count--;

				if (auto ret = data->superblock_write_back(fs);ret != ERROR_SUCCESS)
				{
					delete[] bitmap_buf;
					return ret;
				}

				delete[] bitmap_buf;
				return ino;
			}

		}
	}

	delete[] bitmap_buf;
	return ERROR_UNKOWN; // Shouldn't reach here
}

error_code ext2_inode_free(file_system::fs_instance* fs, file_system::ext2_ino_type ino, bool is_dir)
{
	auto data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	ext2_superblock& superblock = data->get_superblock();
	uint64_t* bitmap_buf = new(std::nothrow)uint64_t[data->get_block_size() / sizeof(uint64_t)];

	auto block = (ino - EXT2_FIRST_INODE_NUMBER) / superblock.block_group_inode_count;
	auto index = (ino - EXT2_FIRST_INODE_NUMBER) % superblock.block_group_inode_count;

	auto bgd = data->get_bgdt() + block;

	auto ret = ext2_block_read(fs, reinterpret_cast<uint8_t*>(bitmap_buf), bgd->block_bitmap_no);
	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	bitmap_buf[index / 64] &= ~(1ull << (index % 64));

	ret = ext2_block_write(fs, reinterpret_cast<const uint8_t*>(bitmap_buf), bgd->block_bitmap_no);
	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	bgd->free_inode_count++;
	bgd->directory_count -= is_dir;

	size_t bgd_block_num = block * sizeof(ext2_block_group_desc) / data->get_block_size();

	ret = ext2_block_write(fs,
		reinterpret_cast<const uint8_t*>(data->get_bgdt()) + bgd_block_num * data->get_block_size(),
		bgd->block_bitmap_no);

	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	superblock.free_inode_count++;
	ret = data->superblock_write_back(fs);
	if (ret != ERROR_SUCCESS)
	{
		delete[] bitmap_buf;
		return ret;
	}

	delete[] bitmap_buf;
	return ERROR_SUCCESS;
}

[[nodiscard]] error_code ext2_inode_read_block(file_system::fs_instance* fs,
	OUT file_system::ext2_inode* inode,
	uint8_t* buf,
	size_t index)
{
	auto block_ret = ext2_inode_get_index(fs, inode, index);

	if (has_error(block_ret))
	{
		return get_error_code(block_ret);
	}

	return ext2_block_read(fs, buf, get_result(block_ret));
}

[[nodiscard]] error_code ext2_inode_write_block(file_system::fs_instance* fs,
	OUT file_system::ext2_inode* inode,
	uint8_t* buf,
	size_t index)
{
	auto block_ret = ext2_inode_get_index(fs, inode, index);
	if (has_error(block_ret))
	{
		return get_error_code(block_ret);
	}

	return ext2_block_write(fs, buf, get_result(block_ret));
}


