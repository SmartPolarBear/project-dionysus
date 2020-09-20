#pragma once

#include "system/types.h"

#include "fs/vfs/vfs.hpp"

namespace file_system
{
	constexpr uint16_t EXT2_SIGNATURE = 0xEF53;

	static inline size_t EXT2_CALC_SIZE(size_t logged_size)
	{
		return 1024u << logged_size;
	}

	enum ext2_superblock_states
	{
		EXT2_STATE_CLEAN,
		EXT2_STATE_HAS_ERROR,
	};

	enum ext2_superblock_error_handling
	{
		EXT2_ERR_HANDLE_IGNORE,
		EXT2_ERR_HANDLE_REMOUNT_READONLY,
		EXT2_ERR_HANDLE_PANIC,
	};

	enum ext2_superblock_operation_system_id
	{
		EXT2_SYSID_LINUX = 0,
		EXT2_SYSID_GNU_HURD = 1,
		EXT2_SYSID_MASIX = 2,
		EXT2_SYSID_FREEBSD = 3,
		EXT2_SYSID_LITES = 4,
	};

	struct ext2_superblock
	{
		// Base superblock
		uint32_t inode_count;
		uint32_t block_count;
		uint32_t reserved_blocks;
		uint32_t free_blocks;
		uint32_t free_inodes;
		uint32_t superblock_no;
		uint32_t log2_block_size;
		uint32_t log2_frag_size;
		uint32_t block_group_blocks;
		uint32_t block_group_frags;
		uint32_t block_group_inodes;
		uint32_t mount_time;
		uint32_t write_time;
		uint16_t mount_count;
		uint16_t fsck_mount_count;
		uint16_t ext2_signature;
		uint16_t state;
		uint16_t error_behavior;
		uint16_t version_minor;
		uint32_t fsck_time;
		uint32_t fsck_interval;
		uint32_t os_id;
		uint32_t version_major;
		uint16_t reserved_uid;
		uint16_t reserved_gid;
		// Extended superblock
		uint32_t first_inode;                   // == 11 in < 1.0
		uint16_t inode_size;                    // == 128 in < 1.0
		uint16_t superblock_block_group;        // For backup
		uint32_t optional_features;             // Don't matter
		uint32_t required_features;             // Cannot read without these
		uint32_t write_required_features;       // Can read without these
		uint8_t filesystem_id[16];
		uint8_t volume_name[16];
		uint8_t last_mount_path[64];
		uint32_t compression;
		uint8_t prealloc_file_blocks;
		uint8_t prealloc_dir_blocks;
		uint16_t __res0;
		uint8_t journal_id[16];
		uint32_t journal_inode;
		uint32_t journal_device;
		uint32_t orphan_inode_list_head;

		// Unused bytes here
	} __attribute__((packed));

	struct ext2_data
	{
		union
		{
			ext2_superblock block;
			uint8_t block_data;
		};
	};

	class ext2_fs_class
		: public fs_class_base
	{
	 public:
		vnode_base* get_root(fs_instance* fs) override;
		error_code initialize(fs_instance* fs, const char* data) override;
		void destroy(fs_instance* fs) override;
		error_code get_vfs_status(fs_instance* fs, vfs_status* ret) override;
	};

	extern ext2_fs_class g_ext2fs;
}