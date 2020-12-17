#include "include/inode.hpp"
#include "include/block.hpp"
#include "include/directory.hpp"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/vnode.hpp"

#include "drivers/cmos/rtc.hpp"

#include <algorithm>

using std::min;
using std::max;

error_code_with_result<file_system::vnode_base*> file_system::ext2_vnode::find(const char* name)
{
	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	auto ext2_fs = this->fs;
	if (ext2_fs == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	ext2_data* data = reinterpret_cast<ext2_data*>(ext2_fs->private_data);

	if (data == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	size_t name_len = strlen(name);
	if (name_len >= VFS_MAX_PATH_LEN)
	{
		return -ERROR_INVALID;
	}

	size_t block_size = data->get_block_size();

	uint8_t* block_buf = new(std::nothrow)  uint8_t[block_size];

	size_t block_off = 0;
	size_t dir_block_count = EXT2_INODE_SIZE(inode) / block_size;

	for (size_t block_idx = 0; block_idx < dir_block_count; block_idx++)
	{
		auto ret = ext2_inode_read_block(ext2_fs, inode, block_buf, block_idx);
		if (ret != ERROR_SUCCESS)
		{
			delete[] block_buf;
			return ret;
		}

		while (block_off < block_size)
		{
			ext2_directory_entry* dir_entry = reinterpret_cast<ext2_directory_entry*>((block_buf + block_off));
			if (dir_entry->ino && (dir_entry->name_length_low == name_len))
			{
				if (strncmp(dir_entry->name, name, name_len) == 0)
				{
					auto alloc_inode_ret = data->create_new_inode();
					if (has_error(alloc_inode_ret))
					{
						delete[] block_buf;
						return get_error_code(alloc_inode_ret);
					}

					ext2_inode* new_inode = get_result(alloc_inode_ret);

					auto err = ext2_inode_read(ext2_fs, dir_entry->ino, new_inode);
					if (err != ERROR_SUCCESS)
					{
						data->free_inode(new_inode);
						delete[] block_buf;
						return err;
					}

					ext2_vnode* vnode = new(std::nothrow)  ext2_vnode(ext2_fs, vnode_types::VNT_DIR, name);
					if (vnode == nullptr)
					{
						delete[] block_buf;
						return -ERROR_MEMORY_ALLOC;
					}

					err = vnode->initialize_from_inode(dir_entry->ino, new_inode);
					if (err != ERROR_SUCCESS)
					{
						delete[] block_buf;
						return err;
					}

					delete[] block_buf;
					return vnode;
				}
			}

			block_off += dir_entry->ent_size;
		}
	}

	delete[] block_buf;
	return -ERROR_NO_ENTRY;
}

error_code_with_result<size_t> file_system::ext2_vnode::read_directory(file_object* fd,
	file_system::directory_entry* entry)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (entry == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ext2data = reinterpret_cast<ext2_data*>(this->fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	size_t full_size = EXT2_INODE_SIZE(inode);
	if (fd->pos >= full_size)
	{
		return 0;
	}

	const size_t block_size = ext2data->get_block_size();

	uint8_t* block_buf = new(std::nothrow) uint8_t[block_size];

	if (block_buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	uint32_t block_idx = fd->pos / block_size;

	if (auto ret = ext2_inode_read_block(this->fs, inode, block_buf, block_idx);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	ext2_directory_entry* dirent = (ext2_directory_entry*)(block_buf + fd->pos % block_size);

	if (dirent->ino == 0)
	{
		fd->pos += dirent->ent_size;

		if (fd->pos / block_size != block_idx)
		{
			if (fd->pos % block_size == 0)
			{
				delete[] block_buf;
				return -ERROR_INVALID;
			}

			return read_directory(fd, entry);
		}

		dirent = reinterpret_cast<ext2_directory_entry*>(block_buf + fd->pos % block_size);

		if (dirent->ino == 0)
		{
			delete[] block_buf;
			return -ERROR_INVALID;
		}
	}

	const auto superblock = ext2data->get_superblock();

	auto name_len =
		EXT2_DIRENT_NAME_LEN(dirent, (superblock.required_features & SBRF_DIRENT_TYPE_FIELD));

	entry->type = DT_UNKNOWN;

	strncpy(entry->name, dirent->name, name_len);

	entry->name[name_len] = '\0';
	entry->off = fd->pos;
	entry->reclen = dirent->ent_size;

	fd->pos += dirent->ent_size;

	return (size_t)entry->reclen;
}

error_code file_system::ext2_vnode::open_directory(file_object* fd)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (fd->vnode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (fd->vnode->get_type() != vnode_types::VNT_DIR)
	{
		return -ERROR_INVALID;
	}

	fd->pos = 0;
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::open(file_object* fd, mode_type opt)
{
	if ((opt & IOCTX_FLG_APPEND) == 1 && (opt & IOCTX_FLG_MASK_ACCESS_MODE) == IOCTX_FLG_RDONLY)
	{
		return -ERROR_INVALID;
	}

	if (opt & IOCTX_FLG_DIRECTORY)
	{
		return -ERROR_INVALID;
	}

	fd->pos = 0;
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::close([[maybe_unused]]const file_system::file_object* fd)
{
	// FIXME:do nothing for now
	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::create(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	auto at_inode = (ext2_inode*)this->private_data;
	if (at_inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto fs_ins = (fs_instance*)this->fs;
	if (fs_ins == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto fs_data = (ext2_data*)fs_ins->private_data;
	if (fs_data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto find_ret = find(filename);
	if (!has_error(find_ret))
	{
		return -ERROR_ALREADY_EXIST;
	}

	if (at_inode->type != EXT2_IFDIR || this->type != vnode_types::VNT_DIR)
	{
		return -ERROR_NOT_DIR;
	}

	auto alloc_inode_ret = fs_data->create_new_inode();
	if (has_error(alloc_inode_ret))
	{
		return get_error_code(alloc_inode_ret);
	}
	ext2_inode* new_inode = get_result(alloc_inode_ret);

	auto alloc_ret = ext2_inode_alloc(fs_ins, false);
	if (has_error(alloc_ret))
	{
		return get_error_code(alloc_ret);
	}
	auto new_inode_id = get_result(alloc_ret);

	new_inode->mtime = new_inode->ctime = new_inode->atime = cmos::cmos_read_rtc_timestamp();

	new_inode->uid = uid;
	new_inode->gid = gid;
	new_inode->flags = mode & 0xFFF;
	new_inode->type = EXT2_IFREG;

	new_inode->hard_link_count = 1;

	if (auto insert_ret =
			ext2_directory_inode_insert(fs_ins, this->inode_id, at_inode, filename, new_inode_id, vnode_types::VNT_REG);
		insert_ret != ERROR_SUCCESS)
	{
		return insert_ret;
	}

	if (auto write_ret = ext2_inode_write(fs_ins, new_inode_id, new_inode);write_ret != ERROR_SUCCESS)
	{
		return write_ret;
	}

	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::make_directory(const char* name, uid_type uid, gid_type gid, size_t mode)
{
	auto at_inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (at_inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (auto ret = find(name);has_error(ret) == false)
	{
		return -ERROR_ALREADY_EXIST;
	}

	if (at_inode->type != EXT2_IFDIR)
	{
		return -ERROR_NOT_DIR;
	}

	ext2_inode* inode = new(std::nothrow) ext2_inode{};
	if (inode == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	uint32_t ino = 0;
	if (auto ret = ext2_inode_alloc(fs, true);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		ino = get_result(ret);
	}

	inode->ctime = inode->atime = inode->mtime = cmos::cmos_read_rtc_timestamp();
	inode->uid = uid;
	inode->gid = gid;
	inode->type = EXT2_IFDIR;
	inode->flags = mode & 0xFFF;
	inode->hard_link_count = 2;// "." and parent directory entry

	// setup . and ..
	if (auto ret = ext2_directory_inode_insert(fs, ino, inode, ".", ino, vnode_types::VNT_DIR);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = ext2_directory_inode_insert(fs, ino, inode, "..", this->inode_id, vnode_types::VNT_DIR);ret
		!= ERROR_SUCCESS)
	{
		return ret;
	}

	// insert to parent
	if (auto ret = ext2_directory_inode_insert(fs, this->inode_id, at_inode, name, ino, vnode_types::VNT_DIR);ret
		!= ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = ext2_inode_write(fs, ino, inode);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	at_inode->hard_link_count++;

	if (auto ret = ext2_inode_write(fs, this->inode_id, at_inode);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::truncate(size_t size)
{
	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (this->fs == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (this->get_type() != vnode_types::VNT_REG)
	{
		return -ERROR_NOT_FILE;
	}

	if (auto ret = ext2_inode_resize(this->fs, inode, size);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	inode->mtime = cmos::cmos_read_rtc_timestamp();

	return ext2_inode_write(this->fs, this->inode_id, inode);
}

error_code file_system::ext2_vnode::unlink()
{
	if (this->inode_id == EXT2_ROOT_DIR_INODE_NUMBER)
	{
		return -ERROR_PERMISSION;
	}

	auto parent = this->parent;
	if (parent == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto ext2data = reinterpret_cast<ext2_data*>(fs->private_data);

	if (ext2data == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto parent_inode = reinterpret_cast<ext2_inode*>(parent->get_private_data());
	if (parent_inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (auto ret = parent->find(this->get_name());has_error(ret))
	{
		return -ERROR_NO_ENTRY;
	}

	if (auto ret = ext2_directory_inode_remove(fs, parent_inode, this);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (inode->type == EXT2_IFDIR)
	{
		parent_inode->hard_link_count--;
		if (auto ret = ext2_inode_write(fs, parent->get_inode_id(), parent_inode);ret != ERROR_SUCCESS)
		{
			return ret;
		}
	}

	if (auto ret = ext2_inode_resize(fs, inode, 0);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	inode->hard_link_count = 0;
	inode->dtime = cmos::cmos_read_rtc_timestamp();

	if (auto ret = ext2_inode_write(fs, this->get_inode_id(), inode);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = ext2_inode_free(fs, this->get_inode_id(), this->get_type() == vnode_types::VNT_DIR);ret
		!= ERROR_SUCCESS)
	{
		return ret;
	}

	// TODO: free vnode?
	return ERROR_SUCCESS;
}

error_code_with_result<offset_t> file_system::ext2_vnode::seek(file_system::file_object* fd,
	size_t offset,
	vfs_seek_methods method)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);

	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	offset_t new_pos = fd->pos;
	switch (method)
	{
	case SM_FROM_START:
		new_pos = offset;
		break;
	case SM_FROM_CURRENT:
		new_pos += offset;
		break;
	case SM_FROM_END:
		new_pos = EXT2_INODE_SIZE(inode) + offset;
		break;
	default:
		return -ERROR_INVALID;
	}

	if (new_pos <= 0)
	{
		return -ERROR_INVALID;
	}

	fd->pos = new_pos;

	return new_pos;
}

error_code file_system::ext2_vnode::stat(OUT file_system::file_status* st)
{
	auto inode = reinterpret_cast<ext2_inode*>(this->private_data);
	auto ext2_fs = this->fs;
	ext2_data* data = reinterpret_cast<ext2_data*>(ext2_fs->private_data);

	if (inode == nullptr || ext2_fs == nullptr || data == nullptr)
	{
		return -ERROR_INTERNAL;
	}

	if (inode->uid != this->uid || inode->gid != this->gid || (inode->flags & 0xFFFull) != this->mode)
	{
		return -ERROR_INTERNAL;
	}

	size_t full_size = EXT2_INODE_SIZE(inode);
	size_t block_size = data->get_block_size();

	st->mode = inode->flags;
	st->uid = inode->uid;
	st->gid = inode->gid;

	st->size = full_size;

	st->blksize = block_size;
	st->blocks = (st->size + block_size - 1) / block_size;

	st->dev = 0;
	st->rdev = 0;

	st->mtime = inode->mtime;
	st->atime = inode->atime;
	st->ctime = inode->ctime;

	st->nlink = inode->hard_link_count;

	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::chmod(size_t mode)
{
	ext2_inode* inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (this->fs == nullptr)
	{
		return -ERROR_INVALID;
	}

	// don't rewrite if mode isn't change
	if (inode->flags == (mode & 0xFFF))
	{
		return ERROR_SUCCESS;
	}

	inode->flags = mode & 0xFFF;
	this->mode = mode & 0xFFF;

	inode->mtime = cmos::cmos_read_rtc_timestamp();

	if (auto ret = ext2_inode_write(fs, this->inode_id, inode);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::chown(uid_type uid, gid_type gid)
{
	ext2_inode* inode = reinterpret_cast<ext2_inode*>(this->private_data);
	if (inode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (this->fs == nullptr)
	{
		return -ERROR_INVALID;
	}

	// don't rewrite if both ids isn't change
	if (inode->uid == uid && inode->gid == gid)
	{
		return ERROR_SUCCESS;
	}

	inode->uid = uid;
	inode->gid = gid;
	this->uid = uid;
	this->gid = gid;

	inode->mtime = cmos::cmos_read_rtc_timestamp();

	if (auto ret = ext2_inode_write(fs, this->inode_id, inode);ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code file_system::ext2_vnode::read_link([[maybe_unused]]char* buf, [[maybe_unused]]size_t lim)
{
	//TODO: implement
	KDEBUG_NOT_IMPLEMENTED;
	return ERROR_SUCCESS;
}

using namespace memory::kmem;
// defined in slab.cc
extern kmem_cache* sized_caches[KMEM_SIZED_CACHE_COUNT];

error_code_with_result<size_t> file_system::ext2_vnode::read(file_system::file_object* fd,
	void* _buf,
	size_t sz)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

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

	size_t full_size = EXT2_INODE_SIZE(inode), has_read = 0;

	if (fd->pos >= full_size)
	{
		return -ERROR_EOF;
	}

	uint8_t* block_buf = new(std::nothrow) uint8_t[data->get_block_size()];
	if (block_buf == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	for (size_t size = min(sz, full_size - fd->pos); size > 0;)
	{
		size_t block_off = fd->pos % data->get_block_size();
		size_t block_idx = fd->pos / data->get_block_size();

		size_t readable = min(size, data->get_block_size() - block_off);

		auto ret = ext2_inode_read_block(ext2_fs, inode, block_buf, block_idx);
		if (ret != ERROR_SUCCESS)
		{
			delete[]block_buf;
			return ret;
		}

		memmove(buf, block_buf + block_off, readable);

		buf += readable;
		fd->pos += readable;
		has_read += readable;

		// To avoid overflow
		if (readable >= size)
		{
			size = 0;
		}
		else
		{
			size -= readable;
		}
	}

	delete[]block_buf;
	return has_read;
}

error_code_with_result<size_t> file_system::ext2_vnode::write(file_system::file_object* fd,
	const void* _buf,
	size_t sz)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

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

	const size_t full_size = EXT2_INODE_SIZE(inode);
	const size_t block_size = data->get_block_size();

	if (fd->pos > full_size)
	{
		return -ERROR_EOF;
	}

	size_t request_size = max(fd->pos + sz, full_size);
	size_t offset = 0;

	if (auto err = ext2_inode_resize(ext2_fs, inode, request_size);err != ERROR_SUCCESS)
	{
		return err;
	}

	uint8_t* block_buf = new(std::nothrow)uint8_t[1024];

	for (offset = 0; sz;)
	{
		auto block_index = fd->pos / block_size;
		auto block_offset = fd->pos % block_size;
		size_t writable = min(sz, block_size - block_offset);

		if (writable == 0)
		{
			delete[] block_buf;
			return -ERROR_INTERNAL;
		}

		if (writable != data->get_block_size())
		{

			if (auto err = ext2_inode_read_block(ext2_fs, inode, block_buf, block_index);err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}

			memmove(block_buf + block_offset, buf + offset, writable);

			if (auto err = ext2_inode_write_block(ext2_fs, inode, block_buf, block_index);err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}
		}
		else
		{
			if (auto err = ext2_inode_write_block(ext2_fs, inode, buf + offset, block_index);err != ERROR_SUCCESS)
			{
				delete[] block_buf;
				return err;
			}
		}

		sz -= writable;
		offset += writable;
		fd->pos += writable;
	}

	inode->mtime = cmos::cmos_read_rtc_timestamp();

	if (auto err = ext2_inode_write(ext2_fs, this->inode_id, inode);err != ERROR_SUCCESS)
	{
		delete[] block_buf;
		return err;
	}

	delete[] block_buf;
	return offset;
}

[[nodiscard]]error_code file_system::ext2_vnode::initialize_from_inode(file_system::ext2_ino_type ino,
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
		this->type = vnode_types::VNT_DIR;
		break;
	}
	case EXT2_IFREG:
	{
		this->type = vnode_types::VNT_REG;
		break;
	}
	default:
	{
		return -ERROR_INVALID;
	}
	}

	return ERROR_SUCCESS;
}

error_code_with_result<file_system::vnode_base*> file_system::ext2_vnode::allocate_new(const char* name,
	gid_type gid,
	uid_type uid,
	mode_type mode)
{
	ext2_vnode* ret = new(std::nothrow) ext2_vnode{ type, name };

	if (ret == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	ret->mode = mode;
	ret->uid = uid;
	ret->gid = gid;

	return ret;
}
