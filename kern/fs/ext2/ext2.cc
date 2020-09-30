#include "include/block.hpp"
#include "include/inode.hpp"

#include "fs/ext2/ext2.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/vnode.hpp"

#include "system/kmalloc.hpp"

#include "drivers/debug/kdebug.h"

using namespace file_system;
using namespace kdebug;
using namespace memory;
using namespace memory::kmem;

ext2_fs_class file_system::g_ext2fs;

ext2_data::ext2_data()
{
	memset(reinterpret_cast<void*>(this->superblock_data), 0, sizeof(this->superblock_data));
}

error_code ext2_data::initialize(fs_instance* fs)
{
	auto ret = fs->dev->read(reinterpret_cast<void*>(&this->superblock_data), 1024, 1024);
	if (get_error_code(ret) != ERROR_SUCCESS)
	{
		return -ERROR_IO;
	}

	// check the signature
	if (this->superblock.ext2_signature != EXT2_SIGNATURE)
	{
		return -ERROR_INVALID;
	}

	// we do not support too old version of ext2 file system
	if (this->superblock.version_major < 1)
	{
		return -ERROR_OBSOLETE;
	}

	block_size = EXT2_CALC_SIZE(this->superblock.log2_block_size);
	fragment_size = EXT2_CALC_SIZE(this->superblock.log2_frag_size);

	this->inode_cache = kmem_cache_create("inode_cache", this->get_inode_size());

	this->inodes_per_block = block_size / get_inode_size();
	this->blkgrp_inode_blocks = superblock.block_group_inode_count / inodes_per_block;
	this->bgdt_entry_count =
		(superblock.block_count + superblock.block_group_block_count - 1) / superblock.block_group_block_count;

	size_t bgdt_size = bgdt_entry_count * sizeof(ext2_block_group_desc);

	this->bgdt_size_blocks = (bgdt_size + block_size - 1) / block_size;

	this->bgdt = reinterpret_cast<ext2_block_group_desc*>(kmalloc(bgdt_size_blocks * block_size, 0));
	if (bgdt == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	// starting from block 2, we read all bgds
	for (size_t i = 0; i < bgdt_size_blocks; i++)
	{
		ret = ext2_block_read(fs, ((uint8_t*)this->bgdt) + i * this->block_size, i + 2);
		if (get_error_code(ret) != ERROR_SUCCESS)
		{
			kfree(this->bgdt);
			return get_error_code(ret);
		}
	}

	// allocate root inode
	root_inode = reinterpret_cast<ext2_inode*>(kmem_cache_alloc(this->inode_cache));

	auto err = ext2_inode_read(fs, EXT2_ROOT_DIR_INODE_NUMBER, root_inode);
	if (err != ERROR_SUCCESS)
	{
		kfree(this->bgdt);
		return err;
	}

	// allocate root
	auto root_vnode = new ext2_vnode{ VNT_DIR, nullptr };
	root = root_vnode;
	if (root == nullptr)
	{
		kfree(this->bgdt);
		return -ERROR_IO;
	}

	root_vnode->initialize_from_inode(EXT2_ROOT_DIR_INODE_NUMBER, root_inode);

	this->print_debug_message();
	return ERROR_SUCCESS;
}

ext2_data::~ext2_data()
{
	kfree(bgdt); // Allocated by kmalloc

	kmem_cache_free(inode_cache, root_inode); // Allocate by the slab allocator

	delete root; // Allocated by new operator

}

void ext2_data::print_debug_message()
{
	kdebug_log("Initializing EXT2 file system version %d.%d on volume %s\n",
		this->superblock.version_major,
		this->superblock.version_minor,

		strnlen(reinterpret_cast<const char*>(this->superblock.volume_name), 256) == 0 ?
		"[no name]" :
		(char*)this->superblock.volume_name);

	kdebug_log(
		"%d blocks, %d inodes, %d reserved for superuser, superblock size %lld, fragment size %lld, %lld bytes in size.\n",
		this->superblock.block_count,
		this->superblock.inode_count,
		this->superblock.reserved_block_count,
		block_size,
		fragment_size,
		block_size * this->superblock.block_count);

	kdebug_log("%lld block groups, %lld for BGDT.\n", bgdt_entry_count, bgdt_size_blocks);
}

error_code ext2_data::superblock_write_back(fs_instance* fs)
{
	auto ret = fs->dev->write(superblock_data, 1024, 1024);
	if (get_error_code(ret) != ERROR_SUCCESS)
	{
		return get_error_code(ret);
	}

	return get_result(ret) == 1024 ? ERROR_SUCCESS : ERROR_IO;
}

error_code_with_result<file_system::vnode_base*> file_system::ext2_fs_class::get_root()
{
	if (this->data == nullptr || this->data->get_root() == nullptr)
	{
		return -ERROR_INVALID;
	}

	return data->get_root();
}

error_code file_system::ext2_fs_class::initialize(fs_instance* fs, [[maybe_unused]] const char* optional_data)
{
	ext2_data* ext2data = nullptr;

	fs->private_data = this->data = ext2data = new ext2_data{};

	if (fs->private_data == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	error_code ret = ext2data->initialize(fs);

	if (ret != ERROR_SUCCESS)
	{
		delete ext2data;
		fs->private_data = nullptr;
		this->data = nullptr;
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code file_system::ext2_fs_class::dispose(fs_instance* fs)
{
	ext2_data* extdata = reinterpret_cast<ext2_data*>(fs->private_data);

	if (extdata == nullptr)
	{
		return -ERROR_INVALID;
	}

	delete extdata;

	return ERROR_SUCCESS;
}

error_code_with_result<vfs_status> file_system::ext2_fs_class::get_vfs_status([[maybe_unused]]fs_instance* fs)
{
	// TODO: refresh the structure
	return this->current_status;
}
