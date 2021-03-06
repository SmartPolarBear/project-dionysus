#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

[[nodiscard]] error_code ext2_inode_read(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	OUT file_system::ext2_inode* inode);

[[nodiscard]] error_code ext2_inode_write(file_system::fs_instance* fs,
	file_system::ext2_ino_type number,
	IN file_system::ext2_inode* inode);

[[nodiscard]] error_code ext2_inode_read_block(file_system::fs_instance* fs,
	OUT file_system::ext2_inode* inode,
	uint8_t *buf,
	size_t index);

[[nodiscard]] error_code ext2_inode_write_block(file_system::fs_instance* fs,
	OUT file_system::ext2_inode* inode,
	uint8_t *buf,
	size_t index);

[[nodiscard]] error_code_with_result<uint32_t> ext2_inode_get_index(file_system::fs_instance* fs,
	IN file_system::ext2_inode* inode,
	uint32_t index);

[[nodiscard]] error_code ext2_inode_set_index(file_system::fs_instance* fs,
	IN file_system::ext2_inode* inode,
	uint32_t index,
	uint32_t value);

[[nodiscard]] error_code_with_result<uint32_t> ext2_inode_alloc(file_system::fs_instance* fs, bool is_dir);

[[nodiscard]] error_code ext2_inode_free(file_system::fs_instance* fs, file_system::ext2_ino_type ino, bool is_dir);

[[nodiscard]] error_code ext2_inode_resize(file_system::fs_instance* fs,
	file_system::ext2_inode* inode,
	size_t new_size
);



