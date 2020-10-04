#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

using namespace file_system;

error_code fs_class_base::register_this()
{
	return fs_register(this);
}

vfs_io_context the_kernel_io_context{ nullptr, 0, 0 };

vfs_io_context* const file_system::kernel_io_context = &the_kernel_io_context;

error_code file_system::vfs_init()
{
	return ERROR_SUCCESS;
}

error_code vfs_io_context::mount_internal(vnode_base* at,
	device_class* blk,
	fs_class_base* cls,
	uint32_t flags,
	const char* opt)
{
	return 0;
}

error_code vfs_io_context::setcwd(const char* rel_path)
{
	return 0;
}

error_code vfs_io_context::vnode_path(char* path, vnode_base* node)
{
	return 0;
}

error_code_with_result<vnode_base*> vfs_io_context::link_resolve(vnode_base* lnk, int link_itself)
{
	return error_code_with_result<vnode_base*>();
}

error_code_with_result<vnode_base*> vfs_io_context::find(vnode_base* rel, const char* path, int link_itself)
{
	return error_code_with_result<vnode_base*>();
}

error_code vfs_io_context::mount(const char* at, device_class* blk, fs_class_id fs_id, size_t flags, const char* opt)
{
	return 0;
}

error_code vfs_io_context::umount(const char* dir_name)
{
	return 0;
}

error_code vfs_io_context::open_vnode(file_object* fd, vnode_base* node, int opt)
{
	return 0;
}

error_code vfs_io_context::openat(file_object* fd, vnode_base* at, const char* path, size_t flags, size_t mode)
{
	return 0;
}

error_code vfs_io_context::close(file_object* fd)
{
	return 0;
}

error_code vfs_io_context::readdir(file_object* fd, directory_entry* ent)
{
	return 0;
}

error_code vfs_io_context::unlinkat(vnode_base* at, const char* pathname, size_t flags)
{
	return 0;
}

error_code vfs_io_context::mkdirat(vnode_base* at, const char* path, mode_type mode)
{
	return 0;
}

error_code_with_result<vnode_base*> vfs_io_context::mknod(const char* path, mode_type mode)
{
	return error_code_with_result<vnode_base*>();
}

error_code vfs_io_context::chmod(const char* path, mode_type mode)
{
	return 0;
}

error_code vfs_io_context::chown(const char* path, uid_type uid, gid_type gid)
{
	return 0;
}

error_code vfs_io_context::ioctl(file_object* fd, size_t cmd, void* arg)
{
	return 0;
}

error_code vfs_io_context::ftruncate(vnode_base* node, size_t length)
{
	return 0;
}

error_code vfs_io_context::faccessat(vnode_base* at, const char* path, size_t accmode, size_t flags)
{
	return 0;
}

error_code vfs_io_context::fstatat(vnode_base* at, const char* path, file_status* st, size_t flags)
{
	return 0;
}

error_code vfs_io_context::access_check(int desm, mode_type mode, uid_type uid, gid_type gid)
{
	return 0;
}

error_code vfs_io_context::access_node(vnode_base* vn, size_t mode)
{
	return 0;
}

error_code_with_result<size_t> vfs_io_context::write(file_object* fd, const void* buf, size_t count)
{
	return error_code_with_result<size_t>();
}

error_code_with_result<size_t> vfs_io_context::read(file_object* fd, void* buf, size_t count)
{
	return error_code_with_result<size_t>();
}

size_t vfs_io_context::lseek(file_object* fd, size_t offset, size_t whence)
{
	return 0;
}
