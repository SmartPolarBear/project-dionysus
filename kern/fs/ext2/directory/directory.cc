#include "../include/directory.hpp"
#include "../include/block.hpp"
#include "../include/inode.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "debug/kdebug.h"

using namespace file_system;

error_code ext2_directory_inode_insert(file_system::fs_instance* fs,
	file_system::ext2_ino_type at_ino,
	file_system::ext2_inode* at_inode,
	const char* name,
	file_system::ext2_ino_type ino,
	file_system::vnode_types type)
{
	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);
	if (data == nullptr)
	{
		return -ERROR_INVALID;
	}

	const size_t block_size = data->get_block_size();

	size_t entry_size = sizeof(ext2_directory_entry) + roundup(strlen(name), 4ul);
	size_t block_count = EXT2_INODE_SIZE(at_inode) / block_size;

	auto buf = new(std::nothrow) uint8_t[data->get_block_size()];
	if (buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ext2_directory_entry* ret = nullptr;
	size_t index = SIZE_MAX;

	for (size_t i = 0; i < block_count; i++)
	{
		auto read_ret = ext2_inode_read_block(fs, at_inode, buf, i);
		if (read_ret != ERROR_SUCCESS)
		{
			delete[] buf;
			return read_ret;
		}

		size_t offset = 0;
		while (offset < block_size)
		{
			ext2_directory_entry* ent = (ext2_directory_entry*)(&buf[offset]);

			size_t size_prev = sizeof(ext2_directory_entry);
			if (ent->ino)
			{
				auto name_len =
					EXT2_DIRENT_NAME_LEN(ent, (data->get_superblock().required_features & SBRF_DIRENT_TYPE_FIELD));

				size_prev += roundup<size_t>(name_len, 4ul);
			}

			if (ent->ent_size < size_prev)
			{
				delete[] buf;
				return -ERROR_INVALID;
			}

			size_t free_space = ent->ent_size - size_prev;
			if (free_space >= entry_size)
			{
				ent->ent_size = size_prev;

				// allocate new entry
				ret = reinterpret_cast<ext2_directory_entry*>(&buf[offset + ent->ent_size]);

				ret->ent_size = free_space;

				index = i;

				break;
			}
			offset += ent->ent_size;
		}

		if (ret != nullptr)
		{
			break;
		}
	}

	bool writeback_need = ret == nullptr;
	if (ret == nullptr)
	{
		// grow the directory
		auto resize_ret = ext2_inode_resize(fs, at_inode, EXT2_INODE_SIZE(at_inode) + block_size);
		if (resize_ret != ERROR_SUCCESS)
		{
			delete[] buf;
			return resize_ret;
		}

		memset(buf, 0, block_size);

		ret = reinterpret_cast<ext2_directory_entry*>(buf);
		ret->ent_size = block_size;

		index = block_count;
	}

	ret->ino = ino;

	auto ret_name_len = strlen(name);
	ret->name_length_low = ret_name_len & 0b11111111U;
	if ((data->get_superblock().required_features & SBRF_DIRENT_TYPE_FIELD) == 0)
	{
		ret->name_length_high = ret_name_len >> 8u;
	}

	strncpy(ret->name,
		name,
		EXT2_DIRENT_NAME_LEN(ret, (data->get_superblock().required_features & SBRF_DIRENT_TYPE_FIELD))
	);

	// set the type indicator
	// TODO: refine this
	if (data->get_superblock().required_features & SBRF_DIRENT_TYPE_FIELD)
	{
		// not extended types
		if (((uint32_t)type) <= 7)
		{
			ret->type_indicator = ((uint32_t)type);
		}
		else
		{
			delete[] buf;
			return -ERROR_INVALID;
		}
	}

	auto write_ret = ext2_inode_write_block(fs, at_inode, buf, index);
	if (write_ret != ERROR_SUCCESS)
	{
		delete[] buf;
		return write_ret;
	}

	if (writeback_need)
	{
		write_ret = ext2_inode_write(fs, at_ino, at_inode);
		if (write_ret != ERROR_SUCCESS)
		{
			delete[] buf;
			return write_ret;
		}
	}

	delete[] buf;
	return ERROR_SUCCESS;
}

error_code ext2_directory_inode_remove(file_system::fs_instance* fs,
	file_system::ext2_inode* at_inode,
	file_system::vnode_base* vn)
{
	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);

	const size_t block_size = data->get_block_size();
	size_t block_count = at_inode->size_lower / block_size;

	auto buf = new uint8_t[block_size];

	ext2_directory_entry* prev = nullptr, * next = nullptr;
	for (size_t i = 0; i < block_count; ++i)
	{
		size_t offset = 0;
		prev = nullptr;

		auto read_ret = ext2_inode_read_block(fs, at_inode, buf, i);
		if (read_ret != ERROR_SUCCESS)
		{
			delete[] buf;
			return read_ret;
		}

		while (offset < block_size)
		{
			ext2_directory_entry* ent = reinterpret_cast<ext2_directory_entry*>(&buf[offset]);

			if (ent->ino == vn->get_inode_id())
			{

				if (strncmp(ent->name,
					vn->get_name(),
					EXT2_DIRENT_NAME_LEN(ent, (data->get_superblock().required_features & SBRF_DIRENT_TYPE_FIELD)))
					!= 0)
				{
					delete[] buf;
					return -ERROR_INVALID;
				}

				ent->ino = 0;

				// FIXME: shift all the following entries to reduce fragmentation
				if (ent->ent_size == block_size)
				{
					// FIXME: Free directory blocks when the last block entry is removed
					return ext2_inode_write_block(fs, at_inode, buf, i);
				}

				if (prev)
				{
					prev->ent_size += ent->ent_size;
					return ext2_inode_write_block(fs, at_inode, buf, i);
				}
				else
				{
					if (offset == 0 || ent->ent_size >= block_size)
					{
						delete[] buf;
						return -ERROR_INVALID;
					}

					next = reinterpret_cast<ext2_directory_entry*>(&buf[offset + ent->ent_size]);
					ent->ent_size = ent->ent_size + next->ent_size;
					memmove(ent, next, next->ent_size);

					auto write_ret = ext2_inode_write_block(fs, at_inode, buf, i);

					delete[] buf;
					return write_ret;
				}
			}

			prev = ent;
			offset += ent->ent_size;
		}
	}

	return -ERROR_NO_ENTRY;
}
