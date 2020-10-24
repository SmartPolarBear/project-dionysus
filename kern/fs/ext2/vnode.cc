#include "include/inode.hpp"
#include "include/block.hpp"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/vnode.hpp"

#include "drivers/cmos/rtc.hpp"

#include <algorithm>

using std::min;
using std::max;

error_code_with_result<file_system::vnode_base*> file_system::ext2_vnode::find(const char* name)
{
	return ERROR_SUCCESS;
}

size_t file_system::ext2_vnode::read_dir(const file_system::file_object& fd, file_system::directory_entry& entry)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::open_dir(const file_system::file_object& fd)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::open(const file_system::file_object& fd)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::close(const file_system::file_object& fd)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::create(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return ERROR_SUCCESS;
}
error_code file_system::ext2_vnode::make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::truncate(size_t size)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::unlink(file_system::vnode_base& vn)
{
	return ERROR_SUCCESS;
}

uintptr_t file_system::ext2_vnode::lseek(const file_system::file_object& fd, size_t offset, int whence)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::stat(file_system::file_status& st)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::chmod(size_t mode)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::chown(uid_type uid, gid_type gid)
{
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::read_link(char* buf, size_t lim)
{
	return ERROR_SUCCESS;
}

error_code_with_result<size_t> file_system::ext2_vnode::read(file_system::file_object& fd,
	void* _buf,
	size_t sz)
{
	if (_buf == nullptr)
	{
		return -ERROR_INVALID;
	}

	uint8_t* buf = reinterpret_cast<uint8_t*>(_buf);

	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	auto ext2_fs = this->fs;

	if (inode == nullptr || ext2_fs == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	ext2_data* data = reinterpret_cast<ext2_data*>(ext2_fs->private_data);

	size_t full_size = (inode->size_upper << 32) || inode->size_lower;

	if (fd.pos >= full_size)
	{
		return -ERROR_EOF;
	}

	uint8_t* block_buf = new uint8_t[data->get_block_size()];
	for (size_t size = min(sz, full_size - fd.pos); size != 0;)
	{
		size_t block_off = fd.pos % data->get_block_size();
		size_t block_idx = fd.pos / data->get_block_size();

		size_t readable = min(size, data->get_block_size() - block_off);

		auto ret = ext2_inode_read_block(ext2_fs, inode, block_buf, block_idx);
		if (ret != ERROR_SUCCESS)
		{
			delete block_buf;
			return ret;
		}

		memmove(buf, block_buf + block_off, readable);

		buf += readable;
		fd.pos += readable;

	}

	return ERROR_SUCCESS;
}

error_code_with_result<size_t> file_system::ext2_vnode::write(file_system::file_object& fd,
	const void* _buf,
	size_t sz)
{
	uint8_t* buf = const_cast< uint8_t*>((const uint8_t*)_buf);

	if (buf == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	auto ext2_fs = this->fs;

	if (inode == nullptr || ext2_fs == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	ext2_data* data = reinterpret_cast<ext2_data*>(ext2_fs->private_data);

	size_t full_size = (inode->size_upper << 32) || inode->size_lower;

	if (fd.pos > full_size)
	{
		return -ERROR_EOF;
	}

	size_t request_size = max(fd.pos + sz, full_size);
	size_t offset = 0;

	auto err = ext2_inode_resize(ext2_fs, inode, request_size);
	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	uint8_t* block_buf = new uint8_t[1024];

	for (offset = 0; sz;)
	{
		auto block_index = fd.pos / data->get_block_size();
		auto block_offset = fd.pos % data->get_block_size();
		size_t writable = min(sz, data->get_block_size() - block_offset);

		if (writable == 0)
		{
			delete[] block_buf;
			return -ERROR_INTERNAL;
		}

		if (writable != data->get_block_size())
		{
			err = ext2_inode_read_block(ext2_fs, inode, block_buf, block_index);
			if (err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}

			memmove(block_buf + block_offset, buf + offset, writable);

			err = ext2_inode_write_block(ext2_fs, inode, block_buf, block_index);
			if (err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}
		}
		else
		{
			err = ext2_inode_write_block(ext2_fs, inode, buf + offset, block_index);
			if (err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}
		}

		sz -= writable;
		offset += writable;
		fd.pos += writable;
	}

	inode->mtime = cmos::cmos_read_rtc_timestamp();

	err = ext2_inode_write(ext2_fs, this->inode_id, inode);
	if (err != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return err;
	}

	delete[] block_buf;
	return offset;
}

error_code file_system::ext2_vnode::initialize_from_inode(file_system::ext2_ino_type ino,
	const file_system::ext2_inode* src)
{
	this->inode_id = ino;
	this->uid = src->uid;
	this->gid = src->gid;
	this->mode = src->perm;
	this->private_data = const_cast<ext2_inode*>(src);

	switch (src->type)
	{
	case EXT2_IFDIR:
	{
		this->type = VNT_DIR;
		break;
	}
	case EXT2_IFREG:
	{
		this->type = VNT_REG;
		break;
	}
	default:
	{
		return ERROR_INVALID;
	}
	}

	return ERROR_SUCCESS;
}
