#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

#include <memory>

using namespace std;

using namespace file_system;

vnode_base* vfs_root = nullptr;

vfs_io_context the_kernel_io_context{ nullptr, 0, 0 };
vfs_io_context* const file_system::kernel_io_context = &the_kernel_io_context;

error_code fs_class_base::register_this()
{
	return fs_register(this);
}

error_code file_system::vfs_init()
{
	vfs_root = nullptr;
	return ERROR_SUCCESS;
}

static inline const char* next_path_element(const char* path, OUT char* element)
{
	auto pos = strchr(path, '/');
	if (pos == nullptr) // no '/', there is only one element left
	{
		strncpy(element, path, VFS_MAX_PATH_LEN); // we can't copy more than path length
		return nullptr;
	}
	else
	{
		strncpy(element, path, pos - path);

		while (*pos == '/') // bypass the / to return
		{
			++pos;
		}

		if (*pos == '\0')
		{
			return nullptr;
		}

		return pos;
	}
}

static inline error_code_with_result<vnode_base*> vfs_lookup_or_load(vnode_base *at,const char*name)
{
	return ERROR_SUCCESS;
}

error_code_with_result<vnode_base*> vfs_io_context::do_find(vnode_base* mount, const char* path, bool link_itself)
{
	auto buf = new char[VFS_MAX_PATH_LEN];
	vnode_base* result = nullptr;

	if (mount == nullptr ||
		path == nullptr ||
		path[0] == 0 ||
		path[0] == '/') // path should neither be empty nor be absolute
	{
		return -ERROR_INVALID;
	}

	if (mount->has_flags(VNT_LNK))
	{
		//TODO: multiple links
		return -ERROR_NOT_IMPL;
	}

	if (!mount->has_flags(VNT_DIR))
	{
		return -ERROR_NOT_DIR;
	}


	// TODO: check access permissions
	auto path_next = next_path_element(path, buf);
	for (;; path_next = next_path_element(path_next, buf))
	{
		if (strcmp(buf, ".") == 0)
		{
			if (path_next == nullptr || path_next[0] == 0)
			{
				result = mount;
				return result;
			}
			continue;
		}
		else if (strcmp(buf, "..") == 0)
		{
			auto parent = mount->get_parent();
			if (parent == nullptr)
			{
				parent = mount;
			}

			return do_find(parent, path_next, link_itself);
		}

		break;
	}



	delete[]buf;
}

error_code vfs_io_context::set_cwd(const char* rel_path)
{
	return 0;
}

error_code vfs_io_context::vnode_path(char* path, vnode_base* node)
{
	return 0;
}

error_code_with_result<vnode_base*> vfs_io_context::link_resolve(vnode_base* lnk, bool link_itself)
{
	return error_code_with_result<vnode_base*>();
}

error_code_with_result<vnode_base*> vfs_io_context::find(vnode_base* rel, const char* path, bool link_itself)
{
	if (vfs_root == nullptr)
	{
		return -ERROR_INVALID;
	}

	vnode_base* find_base = nullptr;
	if (path[0] == '/')
	{
		while (*path == '/')path++;

		rel = vfs_root;
	}
	else
	{
		if (rel == nullptr)
		{
			rel = this->cwd_vnode;
		}

		if (rel == nullptr)
		{
			rel = vfs_root;
		}
	}

}

error_code vfs_io_context::mount(const char* path, device_class* blk, fs_class_id fs_id, size_t flags, const char* opt)
{
	if (blk == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (path[0] != '/')
	{
		return -ERROR_INVALID;
	}

	fs_class_base* fs_class = fs_find(fs_id);

	if (fs_class == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (blk->has_flag(BLOCKDEV_BUSY))
	{
		return -ERROR_DEV_BUSY;
	}

	auto ret = fs_create(fs_class, blk, flags, opt);
	if (get_error_code(ret) != ERROR_SUCCESS)
	{
		return get_error_code(ret);
	}

	auto fs_ins = get_result(ret);

	auto root_ret = fs_class->get_root();
	if (get_error_code(root_ret) != ERROR_SUCCESS)
	{
		return get_error_code(root_ret);
	}

	auto fs_root = get_result(root_ret);

	if (strcmp(path, "/") == 0) // mount root
	{
		vfs_root = fs_root;
	}
	else
	{
		auto mount_point_ret = this->find(this->cwd_vnode, path, false);
		if (get_error_code(mount_point_ret) == ERROR_SUCCESS)
		{
			return get_error_code(mount_point_ret);
		}

		auto mountpoint = get_result(mount_point_ret);

		mountpoint->set_flags(VNT_MNT);
		mountpoint->set_link_target(fs_root);

		fs_root->set_link_target(mountpoint);
		fs_root->set_parent(mountpoint);
	}

	blk->set_flags(blk->get_flags() | BLOCKDEV_BUSY);

	// TODO: Do fs_class needs its own initialization?

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

