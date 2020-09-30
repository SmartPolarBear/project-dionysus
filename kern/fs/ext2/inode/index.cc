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
	return ERROR_SUCCESS;
}

error_code ext2_inode_set_index(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	file_system::ext2_inode* inode,
	uint32_t index,
	uint32_t value)
{
	return 0;
}

