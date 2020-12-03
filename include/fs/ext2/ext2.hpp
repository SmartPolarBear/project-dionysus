#pragma once

#include "system/types.h"
#include "system/error.hpp"
#include "system/kmem.hpp"

#include "fs/vfs/vfs.hpp"

#include <optional>

namespace file_system
{
	constexpr uint16_t EXT2_SIGNATURE = 0xEF53;
	constexpr size_t EXT2_DIRECT_BLOCK_COUNT = 12;

	enum ext2_inode_type
	{
		EXT2_IFIFO = 0x1,
		EXT2_IFCHR = 0x2,
		EXT2_IFDIR = 0x4,
		EXT2_IFBLK = 0x6,
		EXT2_IFREG = 0x8, // regular file
		EXT2_IFLNK = 0xA,
		EXT2_IFSOCK = 0xC,
	};

	using ext2_ino_type = uint32_t;
	using block_address_type = uint32_t;

	constexpr ext2_ino_type EXT2_FIRST_INODE_NUMBER = 1;
	constexpr ext2_ino_type EXT2_ROOT_DIR_INODE_NUMBER = 2;

	static inline constexpr size_t EXT2_CALC_SIZE(size_t logged_size)
	{
		return 1024u << logged_size;
	}

	static inline constexpr size_t EXT2_INODE_GET_BLOCK_GROUP(ext2_ino_type ino, size_t block_group_inode_count)
	{
		return (ino - 1) / block_group_inode_count;
	}

	static inline constexpr size_t EXT2_INODE_INDEX_IN_BLOCK_GROUP(ext2_ino_type ino, size_t block_group_inode_count)
	{
		return (ino - 1) % block_group_inode_count;
	}

	static inline constexpr size_t EXT2_BLOCK_GET_BLOCK_GROUP(uint64_t blk, size_t block_group_blocks)
	{
		return (blk - 1) / block_group_blocks;
	}

	static inline constexpr size_t EXT2_BLOCK_INDEX_IN_BLOCK_GROUP(uint64_t blk, size_t block_group_blocks)
	{
		return (blk - 1) % block_group_blocks;
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

	enum ext2_superblock_required_feature_flags
	{
		SBRF_COMPRESSION = 0x0001,
		SBRF_DIRENT_TYPE_FIELD = 0x002,
		SBRF_NEED_REPLAY_JOURNAL = 0x004,
		SBRF_USE_JOURNAL_DEVICE = 0x008
	};

	enum ext2_superblock_optional_feature_flags
	{
		SBOF_BLOCK_PREALLOCATE = 0x0001,
		SBOF_AFS_SERVER_INODE = 0x0002,
		SBOF_JOURNAL = 0x0004,
		SBOF_INODE_EXTENDED_ATTRIBUTES = 0x0008,
		SBOF_SELF_RESIZE_FOR_LARGER_PART = 0x0010,
		SBOF_DIR_USE_HASH_INDEX = 0x0020,
	};

	enum ext2_superblock_write_required_feature_flags
	{
		SBROF_SPARSE_SB_GDT = 0x0001,
		SBROF_64BIT_FILE_SIZE = 0x0002,
		SBROF_DIR_CONTENT_USE_BINARY_TREE = 0x0004
	};

	struct ext2_superblock
	{
		// Base superblock
		uint32_t inode_count;
		uint32_t block_count;
		uint32_t reserved_block_count;
		uint32_t free_block_count;
		uint32_t free_inode_count;
		uint32_t superblock_no;
		uint32_t log2_block_size;
		uint32_t log2_frag_size;
		uint32_t block_group_block_count;
		uint32_t block_group_frag_count;
		uint32_t block_group_inode_count;
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
		uint32_t first_inode;
		uint16_t inode_size;
		uint16_t superblock_block_group;

		uint32_t optional_features;
		uint32_t required_features;
		uint32_t write_required_features;

		uint8_t filesystem_id[16];
		uint8_t volume_name[16];
		uint8_t last_mount_path[64];
		uint32_t compression;
		uint8_t prealloc_file_block_count;
		uint8_t prealloc_dir_block_count;
		uint16_t reserved0;
		uint8_t journal_id[16];
		uint32_t journal_inode;
		uint32_t journal_device;
		uint32_t orphan_inode_list_head;

		// Unused bytes here
	} __attribute__((packed));

	struct ext2_block_group_desc
	{
		uint32_t block_bitmap_no;
		uint32_t inode_bitmap_no;
		uint32_t inode_table_no;
		uint16_t free_block_count;
		uint16_t free_inode_count;
		uint16_t directory_count;
		uint8_t padding0[14];
	} __attribute__((packed));

	struct ext2_inode
	{
		uint64_t perm: 12;
		uint64_t type: 4;
		uint16_t uid;
		uint32_t size_lower;
		uint32_t atime;
		uint32_t ctime;
		uint32_t mtime;
		uint32_t dtime;
		uint16_t gid;
		uint16_t hard_link_count;
		uint32_t sector_count;
		uint32_t flags;
		uint32_t os_val1;
		uint32_t direct_blocks[EXT2_DIRECT_BLOCK_COUNT];
		uint32_t indirect_block_l1;
		uint32_t indirect_block_l2;
		uint32_t indirect_block_l3;
		uint32_t generation;
		uint32_t facl;
		union
		{
			uint32_t size_upper;
			uint32_t dir_acl;
		};
		uint32_t frag_block_no;
		uint32_t os_val2;
	} __attribute__((packed));

	struct ext2_directory_entry
	{
		uint32_t ino;
		uint16_t ent_size;
		uint8_t name_length_low;
		union
		{
			uint8_t type_indicator;
			uint8_t name_length_high;
		};
		char name[0];
	} __attribute__((packed));

	static inline constexpr size_t EXT2_DIRENT_NAME_LEN(const ext2_directory_entry* dirent, bool has_type_indicator)
	{
		return has_type_indicator
			   ? dirent->name_length_low
			   : ((((size_t)dirent->name_length_high) << 32ull) | ((size_t)dirent->name_length_low));
	}

	static inline constexpr size_t EXT2_INODE_SIZE(const ext2_inode* inode)
	{
		return (((uint64_t)inode->size_upper) << 32ull) | ((uint64_t)inode->size_lower);
	}

	static inline void ext2_inode_set_size(ext2_inode* inode, size_t sz)
	{
		inode->size_lower = sz & 0xFFFFFFFFull;
		inode->size_upper = sz >> 32ull;
	}

	static inline constexpr size_t ADDR_COUNT_PER_BLOCK(size_t block_size)
	{
		return block_size / sizeof(block_address_type);
	}

	class ext2_data
	{
	 private:
		using optional_size_t = std::optional<size_t>;
	 private:
		static constexpr size_t EXT2_VERSION0_INODE_SIZE = 128;
	 private:
		union
		{
			ext2_superblock superblock{};
			uint8_t superblock_data[1024];
		};
		void print_debug_message();

		size_t block_size{};
		size_t fragment_size{};
		size_t inodes_per_block{};

		size_t blkgrp_inode_blocks{};

		size_t bgdt_entry_count{};

		size_t bgdt_size_blocks{};

		ext2_block_group_desc* bgdt{};

		vnode_base* root{};

		ext2_inode* root_inode{};
		memory::kmem::kmem_cache* inode_cache{};

	 public:
		[[nodiscard]]  ext2_block_group_desc& get_bgd_by_index(size_t index)
		{
			return bgdt[index];
		}

		[[nodiscard]]  ext2_superblock& get_superblock()
		{
			return superblock;
		}

		[[nodiscard]] size_t get_inodes_per_block() const
		{
			return inodes_per_block;
		}

		[[nodiscard]]  size_t get_inode_size() const
		{
			if (superblock.version_major == 0)
			{
				return EXT2_VERSION0_INODE_SIZE;
			}

			return superblock.inode_size;
		}

		[[nodiscard]] vnode_base* get_root() const
		{
			return root;
		}

		[[nodiscard]] size_t get_block_size() const
		{
			return block_size;
		}

		[[nodiscard]] size_t get_bgdt_entry_count() const
		{
			return bgdt_entry_count;
		}

		[[nodiscard]] ext2_block_group_desc* get_bgdt() const
		{
			return bgdt;
		}

		[[nodiscard]] error_code_with_result<ext2_inode*> create_new_inode();
		void free_inode(ext2_inode* nd);
	 public:
		ext2_data();
		~ext2_data();

		error_code initialize(fs_instance* fs);
		error_code superblock_write_back(fs_instance* fs);

	};

	class ext2_fs_class
		: public fs_class_base
	{
	 private:
		ext2_data* data;

		vfs_status current_status{};
	 public:
		error_code_with_result<vnode_base*> get_root() override;
		error_code_with_result<vfs_status> get_vfs_status(fs_instance* fs) override;

		error_code initialize(fs_instance* fs, const char* data) override;
		error_code dispose(fs_instance* fs) override;

	};

	extern ext2_fs_class g_ext2fs;
}