#include "include/block.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;

error_code ext2_block_read(file_system::fs_instance* fs, uint8_t* buf, size_t count)
{
	ext2_data* ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ret = fs->dev->read(buf, count * ext2data->get_block_size(), ext2data->get_block_size());
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