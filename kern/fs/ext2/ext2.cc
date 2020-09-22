#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;
using namespace kdebug;

ext2_fs_class file_system::g_ext2fs;

static inline error_code ext2_read_superblock(fs_instance* fs)
{
	if (fs->private_data == nullptr)
	{
		return -ERROR_INVALID;
	}

	ext2_data* data = reinterpret_cast<ext2_data*>(fs->private_data);

	auto ret = fs->dev->read(reinterpret_cast<void*>(&data->block_data), 1024, 1024);
	if (ret != ERROR_SUCCESS)
	{
		return -ERROR_IO;
	}

	return ERROR_SUCCESS;
}

static inline void ext2_print_debug_message(const ext2_data* ext2data)
{
	kdebug_log("Initializing EXT2 file system version %d.%d on volume %s\n",
		ext2data->block.version_major,
		ext2data->block.version_minor,

		strnlen(reinterpret_cast<const char*>(ext2data->block.volume_name), 256) == 0 ?
		"[no name]" :
		(char*)ext2data->block.volume_name);

	kdebug_log(
		"%d blocks, %d inodes, %d reserved for superuser, block size %lld, fragment size %lld, %lld bytes in size.\n",
		ext2data->block.block_count,
		ext2data->block.inode_count,
		ext2data->block.reserved_blocks,
		EXT2_CALC_SIZE(ext2data->block.log2_block_size),
		EXT2_CALC_SIZE(ext2data->block.log2_frag_size),
		EXT2_CALC_SIZE(ext2data->block.log2_block_size) * ext2data->block.block_count);
}

error_code_with_result<file_system::vnode_base*> file_system::ext2_fs_class::get_root()
{
	if (this->data == nullptr || this->data->root == nullptr)
	{
		return -ERROR_INVALID;
	}

	return data->root;
}

error_code file_system::ext2_fs_class::initialize(fs_instance* fs, [[maybe_unused]] const char* optional_data)
{
	ext2_data* ext2data = nullptr;
	fs->private_data = this->data = ext2data = new ext2_data{};

	if (fs->private_data == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	error_code ret = ext2_read_superblock(fs);

	if (ret != ERROR_SUCCESS)
	{
		delete ext2data;
		fs->private_data = nullptr;
		return ret;
	}

	// check the signature
	if (ext2data->block.ext2_signature != EXT2_SIGNATURE)
	{
		delete ext2data;
		fs->private_data = nullptr;
		return -ERROR_INVALID;
	}

	// valid ext2 filesystem, print debug message
	ext2_print_debug_message(ext2data);

	return ERROR_SUCCESS;
}

error_code file_system::ext2_fs_class::destroy(fs_instance* fs)
{
	ext2_data* extdata = reinterpret_cast<ext2_data*>(fs->private_data);

	if (extdata == nullptr)
	{
		return -ERROR_INVALID;
	}

	delete extdata->root;
	delete extdata;

	return ERROR_SUCCESS;
}

error_code_with_result<vfs_status> file_system::ext2_fs_class::get_vfs_status([[maybe_unused]]fs_instance* fs)
{
	// TODO: return vfs_status
	return 0;
}

