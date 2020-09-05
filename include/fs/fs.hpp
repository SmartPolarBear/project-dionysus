#pragma once
#include "system/types.h"
#include "fs/mbr.hpp"
#include <cstring>

namespace file_system
{
	struct directory_entry
	{
		uint64_t ino;
		uintptr_t off;
		uint16_t reclen;
		uint8_t type;
		char name[0];
	};

	struct file_object
	{
		size_t flags;
		size_t ref;
		size_t pos;

		class IVNode* vnode;
		void* private_data;
	};

	struct file_status
	{
		uint32_t dev;
		uint32_t ino;
		size_t mode;
		uint32_t nlink;
		uid_type uid;
		gid_type gid;
		uint32_t rdev;
		uint32_t size;
		uint32_t blksize;
		uint32_t blocks;
		size_t atime;
		size_t mtime;
		size_t ctime;
	};

	enum vnode_type
	{
		VNT_REG,
		VNT_DIR,
		VNT_BLK,
		VNT_CHR,
		VNT_LNK,
		VNT_FIFO,
		VNT_SOCK,
		VNT_UNK,
		VNT_MNT,
	};

	enum mode_values : mode_type
	{
		S_IFMT = 0170000,
		S_IFSOCK = 0140000,
		S_IFLNK = 0120000,
		S_IFREG = 0100000,
		S_IFBLK = 0060000,
		S_IFDIR = 0040000,
		S_IFCHR = 0020000,
		S_IFIFO = 0010000,
	};

	PANIC void fs_init();
}