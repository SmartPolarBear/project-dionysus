#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/vnode.hpp"

error_code file_system::ext2_vnode::find(const char* name, file_system::vnode_base& ret)
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

size_t file_system::ext2_vnode::read(const file_system::file_object& fd, void* buf, size_t count)
{
	return ERROR_SUCCESS;
}

size_t file_system::ext2_vnode::write(const file_system::file_object& fd, const void* buf, size_t count)
{
	return ERROR_SUCCESS;
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
