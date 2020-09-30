#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "fs/vfs/vfs.hpp"

[[nodiscard]]error_code ext2_block_read(file_system::fs_instance* fs, uint8_t* buf, size_t block_num);
[[nodiscard]]error_code ext2_block_write(file_system::fs_instance* fs, const uint8_t* buf, size_t block_num);
[[nodiscard]]error_code_with_result<uint64_t> ext2_block_alloc(file_system::fs_instance* fs);
[[nodiscard]]error_code ext2_block_free(file_system::fs_instance* fs, uint32_t block);


