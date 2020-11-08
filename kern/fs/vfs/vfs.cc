#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmem.hpp"

#include <memory>
#include <cstring>

using namespace std;

using namespace file_system;

static inline error_code_with_result<const char*> separate_parent_name(const char* full_path, OUT char* path)
{
	auto sep = strrchr(full_path, '/');

	if (strlen(full_path) >= VFS_MAX_PATH_LEN || strlen(full_path) <= 0)
	{
		return -ERROR_INVALID;
	}

	// path name like "file.ext"
	if (sep == nullptr)
	{
		path[0] = '.';
		path[1] = '\0';

		return full_path;
	}
		// path name like "/a/b/c/file.ext
	else if (sep == full_path)
	{
		path[0] = '/';
		path[1] = '\0';

		return full_path + 1;
	}
	else
	{
		if ((sep - full_path) < (int64_t)VFS_MAX_PATH_LEN)
		{
			return -ERROR_INVALID;
		}

		strncpy(path, full_path, sep - full_path);
		path[sep - full_path] = '\0';
		return full_path + 1;
	}
}

vnode_base* vfs_root = nullptr;

error_code fs_class_base::register_this()
{
	return fs_register(this);
}

error_code file_system::vfs_init()
{
	vfs_root = nullptr;

	auto ret = vnode_init();
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

static constexpr size_t get_open_mode(size_t opt)
{
	size_t r = 0;
	if (opt & IOCTX_FLG_EXEC)
	{
		r |= X_OK;
	}

	switch (opt & IOCTX_FLG_MASK_ACCESS_MODE)
	{
	case IOCTX_FLG_WRONLY:
		r |= W_OK;
		break;
	case IOCTX_FLG_RDONLY:
		r |= R_OK;
		break;
	case IOCTX_FLG_RDWR:
		r |= W_OK | R_OK;
		break;
	}
	return r;
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

static inline error_code_with_result<vnode_base*> vfs_lookup_or_load(vnode_base* at, const char* name)
{
	auto child_ret = at->lookup_child(name);

	auto ret = get_error_code(child_ret);
	if (ret == ERROR_SUCCESS)
	{
		return ret;
	}
	else if (ret != -ERROR_NO_ENTRY)
	{
		return ret;
	}

	if (!(at->has_flags(VNF_MEMORY)))
	{
		auto find_ret = at->find(name);

		if (get_error_code(find_ret) != ERROR_SUCCESS)
		{
			return get_error_code(find_ret);
		}

		auto node = get_result(find_ret);
		at->attach(node);

		return node;
	}

	return -ERROR_NO_ENTRY;
}

error_code vfs_io_context::open_directory(file_object& fd)
{
	if (fd.vnode->get_type() != vnode_types::VNT_DIR)
	{
		return -ERROR_NOT_DIR;
	}

	if (fd.vnode->has_flags(VNF_MEMORY))
	{
		fd.flags |= FO_FLAG_MEMDIR | FO_FLAG_MEMDIR_DOT;
		// FIXME
//		fd.pos = (size_t)node->first_child;

		fd.vnode->increase_open_count();
	}
	else
	{
		auto ret = fd.vnode->open_dir(fd);

		if (ret != ERROR_SUCCESS)
		{
			return ret;
		}

		fd.vnode->increase_open_count();
	}

	return ERROR_SUCCESS;
}

error_code_with_result<vnode_base*> vfs_io_context::do_find(vnode_base* mount, const char* path, bool link_itself)
{
//	auto buf = make_unique<char[]>(VFS_MAX_PATH_LEN);

	if (mount == nullptr ||
		path == nullptr ||
		path[0] == 0 ||
		path[0] == '/') // path should neither be empty nor be absolute
	{
		return -ERROR_INVALID;
	}

	if (mount->get_type() == (vnode_types::VNT_LNK))
	{
		//TODO: multiple links
		return -ERROR_NOT_IMPL;
	}

	if (mount->get_type() != (vnode_types::VNT_DIR))
	{
		return -ERROR_NOT_DIR;
	}


	// TODO: check access permissions

	auto buf = new char[VFS_MAX_PATH_LEN];
	vnode_base* result = nullptr;

	auto path_next = next_path_element(path, buf);
	for (;; path_next = next_path_element(path_next, buf))
	{
		if (strcmp(buf, ".") == 0)
		{
			if (path_next == nullptr || path_next[0] == 0)
			{
				result = mount;

				delete[] buf;
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

			delete[] buf;
			return do_find(parent, path_next, link_itself);
		}

		break;
	}

	auto load_ret = vfs_lookup_or_load(mount, buf);
	if (has_error(load_ret))
	{
		return get_error_code(load_ret);
	}

	result = get_result(load_ret);

	if (path_next != nullptr)
	{
		vnode_base* r = result;
		if (link_itself == false)
		{

			if (r->get_type() == vnode_types::VNT_LNK)
			{
				auto lres_ret = link_resolve(r, false);
				if (has_error(lres_ret))
				{
					return get_error_code(lres_ret);
				}

				r = get_result(lres_ret);
			}

			// TODO: support multiple links
			if (r->get_type() == vnode_types::VNT_LNK)
			{
				return -ERROR_NOT_IMPL;
			}
		}

		return r;
	}
	else
	{
		return do_find(result, path_next, link_itself);
	}

	// Shouldn't reach here
	return -ERROR_NO_ENTRY;
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

	vnode_base* vnode_ret;
	if (path[0] == '/')
	{
		while (*path == '/')path++;

		rel = vfs_root;
		auto find_ret = do_find(vfs_root, path, link_itself);
		if (has_error(find_ret))
		{
			return get_error_code(find_ret);
		}

		vnode_ret = get_result(find_ret);
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

		auto find_ret = do_find(vfs_root, path, link_itself);
		if (has_error(find_ret))
		{
			return get_error_code(find_ret);
		}

		vnode_ret = get_result(find_ret);

	}

	if (vnode_ret->get_type() == vnode_types::VNT_MNT)
	{
		vnode_ret = vnode_ret->get_link_target();
	}

	return vnode_ret;
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

		mountpoint->set_type(vnode_types::VNT_MNT);
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

error_code vfs_io_context::open_vnode(file_object& fd, vnode_base* node, mode_type opt)
{
	if (node == nullptr)
	{
		return -ERROR_INVALID;
	}

	// TODO: network socket file support?

	if (opt & IOCTX_FLG_DIRECTORY)
	{
		if (node->get_type() != vnode_types::VNT_DIR)
		{
			return -ERROR_NOT_DIR;
		}

		if ((opt & IOCTX_FLG_MASK_ACCESS_MODE) != IOCTX_FLG_RDONLY)
		{
			return -ERROR_INVALID_ACCESS;
		}

		fd.pos = 0;
		fd.vnode = node;
		fd.flags = FO_FLAG_DIRECTORY | FO_FLAG_READABLE;

		auto err = this->open_directory(fd);
		if (err != ERROR_SUCCESS)
		{
			return err;
		}

		return ERROR_SUCCESS;
	}
	else if (node->get_type() == vnode_types::VNT_DIR || node->get_type() == vnode_types::VNT_MNT)
	{
		return -ERROR_IS_DIR;
	}

	auto access_mode = get_open_mode(opt);
	auto err = access_node(node, access_mode);
	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	if (opt & IOCTX_FLG_TRUNC)
	{
		err = node->truncate(0);
		if (err != ERROR_SUCCESS)
		{
			return err;
		}
	}

	fd.pos = 0;
	fd.vnode = node;
	fd.flags = 0;

	if ((opt & ~IOCTX_FLG_MASK_ACCESS_MODE) & ~(IOCTX_FLG_CLOEXEC | IOCTX_FLG_CREATE | IOCTX_FLG_TRUNC))
	{
		return -ERROR_INVALID;
	}

	switch (opt & IOCTX_FLG_MASK_ACCESS_MODE)
	{
	case IOCTX_FLG_RDONLY:
		fd.flags |= FO_FLAG_READABLE;
		break;
	case IOCTX_FLG_WRONLY:
		fd.flags |= FO_FLAG_WRITABLE;
		break;
	case IOCTX_FLG_RDWR:
		fd.flags |= FO_FLAG_READABLE | FO_FLAG_WRITABLE;
		break;
	}
	if (opt & IOCTX_FLG_CLOEXEC)
	{
		fd.flags |= FO_FLAG_CLOEXEC;
	}

	// Here the vnode can choose not to support this, so we see ERROR_UNSUPPORTED as normal.
	err = node->open(fd, opt);
	if (err != ERROR_SUCCESS && err != ERROR_UNSUPPORTED)
	{
		return err;
	}

	fd.vnode->increase_open_count();

	return ERROR_SUCCESS;
}

error_code vfs_io_context::create_at(vnode_base* at, const char* full_path, mode_type mode)
{
	if (full_path == nullptr)
	{
		return -ERROR_INVALID;
	}

	char* path = new char[VFS_MAX_PATH_LEN];
	auto name_ret = separate_parent_name(full_path, path);
	if (has_error(name_ret))
	{
		return get_error_code(name_ret);
	}

	auto name = get_result(name_ret);

	auto find_ret = this->find(at, path, false);
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}

	auto vnode_at = get_result(find_ret);

	auto access_ret = this->access_node(vnode_at, W_OK);
	if (access_ret != ERROR_SUCCESS)
	{
		return access_ret;
	}

	return vnode_at->create(name, this->uid, this->gid, mode & ~this->mode_mask);
}

error_code vfs_io_context::open_at(file_object& fd, vnode_base* at, const char* path, size_t flags, size_t mode)
{
	if (at == nullptr)
	{
		at = this->cwd_vnode;
	}

	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto find_ret = this->find(at, path, false);
	if (has_error(find_ret))
	{
		if (flags & IOCTX_FLG_CREATE)
		{
			if (flags & IOCTX_FLG_DIRECTORY)
			{
				return -ERROR_INVALID;
			}

			// try creating
			auto create_ret = this->create_at(at, path, mode);
			if (create_ret != ERROR_SUCCESS)
			{
				return create_ret;
			}

			find_ret = this->find(at, path, false);
			if (has_error(find_ret))
			{
				return get_error_code(find_ret);
			}
		}
		else
		{
			return get_error_code(find_ret);
		}
	}
	auto node = get_result(find_ret);

	return this->open_vnode(fd, node, flags);
}

error_code vfs_io_context::close(file_object& fd)
{
	return 0;
}

error_code vfs_io_context::readdir(file_object& fd, directory_entry* ent)
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

error_code vfs_io_context::ioctl(file_object& fd, size_t cmd, void* arg)
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

error_code_with_result<size_t> vfs_io_context::write(file_object& fd, const void* buf, size_t count)
{
	return error_code_with_result<size_t>();
}

error_code_with_result<size_t> vfs_io_context::read(file_object& fd, void* buf, size_t count)
{
	return error_code_with_result<size_t>();
}

size_t vfs_io_context::seek(file_object& fd, size_t offset, size_t whence)
{
	return 0;
}

