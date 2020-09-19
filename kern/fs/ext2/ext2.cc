#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;

ext2_fs_class file_system::g_ext2fs;

static inline error_code ext2_read_superblock(fs_instance* fs)
{
	if (fs->private_data == nullptr)
	{
		return -ERROR_INVALID;
	}

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);

	size_t read_size = fs->dev->read(reinterpret_cast<void*>(&data->block_data), 1024, 1024);
	if (read_size != 1024)
	{
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}

file_system::vnode_base* file_system::ext2_fs_class::get_root(fs_instance* fs)
{
	return nullptr;
}

error_code file_system::ext2_fs_class::initialize(fs_instance* fs, const char* data)
{
	ext2_data* ext2data = nullptr;
	fs->private_data = ext2data = new ext2_data{};

	if (fs->private_data == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	error_code ret = ext2_read_superblock(fs);
	if (ret != ERROR_SUCCESS)
	{
		delete fs->private_data;
		fs->private_data = nullptr;
		return ret;
	}

	// check the signature
	if (ext2data->block.ext2_signature != EXT2_SIGNATURE)
	{
		delete fs->private_data;
		fs->private_data = nullptr;
		return -ERROR_INVALID;
	}

	return 0;
}

void file_system::ext2_fs_class::destroy(fs_instance* fs)
{

}

error_code file_system::ext2_fs_class::get_vfs_status(fs_instance* fs, vfs_status* ret)
{
	return 0;
}

