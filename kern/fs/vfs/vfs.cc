#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

#include "system/kmem.hpp"

#include "ktl/stack.hpp"

#include <memory>
#include <cstring>

//FIXME: better task ERROR_UNSUPPORTED
//TODO: add locks

using namespace ktl;

using namespace file_system;

vnode_base* vfs_root = nullptr;

error_code fs_class_base::register_this()
{
	return fs_register(this);
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

// FIXME: weird page fault caused by library function.
char*
strrchr2(const char* s,
	int i)
{
	const char* last = NULL;

	if (i)
	{
		while ((s = strchr(s, i)))
		{
			last = s;
			s++;
		}
	}
	else
	{
		last = strchr(s, i);
	}

	return (char*)last;
}

error_code_with_result<const char*> vfs_io_context::separate_parent_name(const char* full_path, OUT char* path)
{
//	auto sep = strrchr(full_path, '/');
	auto sep = strrchr2(full_path, '/');

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

const char* vfs_io_context::next_path_element(const char* path, OUT char* element)
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

error_code_with_result<vnode_base*> vfs_io_context::do_link_resolve(vnode_base* lnk, bool link_itself, size_t count)
{
	if (lnk == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (lnk->get_type() != vnode_types::VNT_LNK)
	{
		return lnk;
	}

	if (lnk->get_link_target() != nullptr && lnk->has_flags(VNF_MEMORY) == false)
	{
		char* path_buf = new(std::nothrow)char[VFS_MAX_PATH_LEN];

		auto err = lnk->read_link(path_buf, VFS_MAX_PATH_LEN);
		if (err != ERROR_SUCCESS)
		{
			delete[] path_buf;
			return err;
		}

		auto find_ret = find(lnk->get_parent(), path_buf, link_itself);

		if (has_error(find_ret))
		{
			lnk->set_link_target(nullptr);

			delete[] path_buf;
			return get_error_code(find_ret);
		}

		delete[] path_buf;
	}

	vnode_base* target = nullptr;
	if (lnk->has_flags(VNF_PER_PROCESS))
	{
		target = lnk->get_link_getter_func()(cur_proc(), lnk, nullptr, 0);
	}
	else
	{
		target = lnk->get_link_target();
	}

	if (target != nullptr)
	{
		if (target->get_type() == vnode_types::VNT_LNK)
		{
			if (count <= 1)
			{
				return -ERROR_TOO_MANY_CALLS;
			}

			return do_link_resolve(target, link_itself, count - 1);
		}
		else
		{
			return target;
		}
	}

	return -ERROR_NO_ENTRY;
}

error_code_with_result<vnode_base*> vfs_io_context::lookup_or_load_node(vnode_base* at, const char* name)
{
	auto child_ret = at->lookup_child(name);

	auto ret = get_error_code(child_ret);
	if (ret == ERROR_SUCCESS)
	{
		return child_ret;
	}
	else if (ret != -ERROR_NO_ENTRY)
	{
		return child_ret;
	}

	if (!(at->has_flags(VNF_MEMORY)))
	{
		auto find_ret = at->find(name);

		if (has_error(find_ret))
		{
			return get_error_code(find_ret);
		}

		auto node = get_result(find_ret);
		at->attach(node);

		return node;
	}

	return -ERROR_NO_ENTRY;
}

error_code vfs_io_context::open_directory(file_object* fd)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (fd->vnode->get_type() != vnode_types::VNT_DIR)
	{
		return -ERROR_NOT_DIR;
	}

	if (fd->vnode->has_flags(VNF_MEMORY))
	{
		fd->flags |= FO_FLAG_PSEUDO_DIR | FO_FLAG_PSEUDO_DIR_DOT;

		fd->pos = (size_t)fd->vnode->get_first();

		fd->vnode->increase_open_count();
	}
	else
	{
		auto ret = fd->vnode->open_directory(fd);

		if (ret != ERROR_SUCCESS)
		{
			return ret;
		}

		fd->vnode->increase_open_count();
	}

	return ERROR_SUCCESS;
}

error_code_with_result<vnode_base*> vfs_io_context::do_find(vnode_base* at, const char* path, bool link_itself)
{
	if (at == nullptr)
	{
		return -ERROR_INVALID;
	}

	// it's nullptr if we have completely solve the path
	if (path == nullptr || path[0] == 0)
	{
		return at;
	}

	if (path[0] == '/')// path should neither be empty nor be absolute
	{
		return -ERROR_INVALID;
	}

	if (at->get_type() == vnode_types::VNT_LNK)
	{
		auto sv_ret = link_resolve(at, link_itself);
		if (has_error(sv_ret))
		{
			return sv_ret;
		}
		else
		{
			at = get_result(sv_ret);
		}
	}

	if (at->get_type() == vnode_types::VNT_LNK)
	{
		// TODO: multiple link
	}

	if (at->get_type() == vnode_types::VNT_MNT)
	{
		at = at->get_link_target();
	}

	if (at->get_type() != (vnode_types::VNT_DIR))
	{
		return -ERROR_NOT_DIR;
	}

	// TODO: check access permissions

	auto buf = new(std::nothrow)char[VFS_MAX_PATH_LEN];
	auto path_next = next_path_element(path, buf);

	vnode_base* result = nullptr;
	for (;; path_next = next_path_element(path_next, buf))
	{
		if (strcmp(buf, ".") == 0)
		{
			if (path_next == nullptr || path_next[0] == 0)
			{
				result = at;

				delete[] buf;
				return result;
			}
			continue;
		}
		else if (strcmp(buf, "..") == 0)
		{
			auto parent = at->get_parent();
			if (parent == nullptr)
			{
				parent = at;
			}

			delete[] buf;
			return do_find(parent, path_next, link_itself);
		}

		break;
	}

	auto load_ret = lookup_or_load_node(at, buf);
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
				auto link_res_ret = link_resolve(r, false);
				if (has_error(link_res_ret))
				{
					return get_error_code(link_res_ret);
				}

				r = get_result(link_res_ret);
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

error_code vfs_io_context::set_cwd(const char* path)
{
	if (strncmp(path, "/", 2) == 0)
	{
		this->cwd_vnode = nullptr;
		return ERROR_SUCCESS;
	}

	if (path[0] == '/')
	{
		if (!vfs_root)
		{
			this->cwd_vnode = nullptr;
		}
		else
		{
			auto find_ret = do_find(vfs_root, path, false);
			if (has_error(find_ret))
			{
				return get_error_code(find_ret);
			}

			auto resolve_ret = link_resolve(get_result(find_ret), false);
			if (has_error(resolve_ret))
			{
				return get_error_code(resolve_ret);
			}

			auto dest = get_result(resolve_ret);

			if (dest->get_type() == vnode_types::VNT_MNT)
			{
				dest = dest->get_link_target();
			}

			if (dest->get_type() != vnode_types::VNT_DIR)
			{
				return -ERROR_NOT_DIR;
			}

			auto err = access_node(dest, X_OK);
			if (err != ERROR_SUCCESS)
			{
				return err;
			}

			this->cwd_vnode = dest;
		}
		return ERROR_SUCCESS;
	}
	else
	{
		auto find_ret = do_find(vfs_root, path, false);
		if (has_error(find_ret))
		{
			return get_error_code(find_ret);
		}

		auto resolve_ret = link_resolve(get_result(find_ret), false);
		if (has_error(resolve_ret))
		{
			return get_error_code(resolve_ret);
		}

		auto dest = get_result(resolve_ret);

		if (dest->get_type() == vnode_types::VNT_MNT)
		{
			dest = dest->get_link_target();
		}

		if (dest->get_type() != vnode_types::VNT_DIR)
		{
			return -ERROR_NOT_DIR;
		}

		auto err = access_node(dest, X_OK);
		if (err != ERROR_SUCCESS)
		{
			return err;
		}

		this->cwd_vnode = dest;

		return ERROR_SUCCESS;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code vfs_io_context::vnode_get_path(char* path, vnode_base* node)
{
	ktl_stack<vnode_base*> backstack;
	if (node == nullptr || node->get_parent() == nullptr)
	{
		path[0] = '/';
		path[0] = 0;
	}

	for (auto n = node; n != nullptr; n = n->get_parent())
	{
		backstack.push(n);
	}

	size_t off = 0;
	while (!backstack.empty())
	{
		auto top = backstack.top();
		backstack.pop();
		path[off++] = '/';
		if (top->get_type() == vnode_types::VNT_DIR && top->get_link_target())
		{
			top = top->get_link_target();
		}

		strncpy(path + off, top->get_name(), VFS_MAX_PATH_LEN);
		off += strlen(top->get_name());
		if (off >= VFS_MAX_PATH_LEN)
		{
			return -ERROR_INVALID;
		}
	}

	return 0;
}

error_code_with_result<vnode_base*> vfs_io_context::link_resolve(vnode_base* lnk, bool link_itself)
{
	return do_link_resolve(lnk, link_itself, LINK_MAX);
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
		return -ERROR_BUSY;
	}

	auto ret = fs_create(fs_class, blk, flags, opt);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	[[maybe_unused]]auto fs_ins = get_result(ret);

	auto root_ret = fs_class->get_root();
	if (has_error(root_ret))
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
		if (has_error(mount_point_ret))
		{
			return get_error_code(mount_point_ret);
		}

		auto mount_point = get_result(mount_point_ret);

		mount_point->set_type(vnode_types::VNT_MNT);
		mount_point->set_link_target(fs_root);

		fs_root->set_link_target(mount_point);
		fs_root->set_parent(mount_point);
	}

	blk->set_flags(blk->get_flags() | BLOCKDEV_BUSY);

	// TODO: Do fs_class needs its own initialization?

	return 0;
}

error_code vfs_io_context::umount(const char* dir_name)
{
	auto find_ret = find(cwd_vnode, dir_name, false);
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}

	auto node = get_result(find_ret);
	if (node->get_parent() == nullptr)
	{
		kdebug::kdebug_warning("Tried to unmount the root!");
		return -ERROR_BUSY;
	}

	if (node->get_type() != vnode_types::VNT_DIR)
	{
		return -ERROR_NOT_DIR;
	}

	if (!node->get_link_target())
	{
		return -ERROR_INVALID;
	}

	auto mount_point = node->get_link_target();
	if (mount_point->get_link_target() != node) // TODO: this can be wrong!
	{
		return -ERROR_INVALID;
	}

	mount_point->set_type(vnode_types::VNT_DIR);
	mount_point->set_link_target(nullptr);

	auto fs = node->get_fs();
	if (fs == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto blk = fs->dev;
	if (blk == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto fs_class = fs->fs_class;
	if (fs_class == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto dispose_ret = fs_class->dispose(fs);
	if (dispose_ret != -ERROR_UNSUPPORTED && dispose_ret != ERROR_SUCCESS)
	{
		return dispose_ret;
	}

	memset(fs, 0, sizeof(fs_instance));
	blk->set_flags(blk->get_flags() & (~BLOCKDEV_BUSY));

	// TODO: destroy filesystem tree

	return ERROR_SUCCESS;
}

error_code vfs_io_context::open_vnode(file_object* fd, vnode_base* node, mode_type opt)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

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

		fd->pos = 0;
		fd->vnode = node;
		fd->flags = FO_FLAG_DIRECTORY | FO_FLAG_READABLE;

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

	fd->pos = 0;
	fd->vnode = node;
	fd->flags = 0;

	if ((opt & ~IOCTX_FLG_MASK_ACCESS_MODE) & ~(IOCTX_FLG_CLOEXEC | IOCTX_FLG_CREATE | IOCTX_FLG_TRUNC))
	{
		return -ERROR_INVALID;
	}

	switch (opt & IOCTX_FLG_MASK_ACCESS_MODE)
	{
	case IOCTX_FLG_RDONLY:
		fd->flags |= FO_FLAG_READABLE;
		break;
	case IOCTX_FLG_WRONLY:
		fd->flags |= FO_FLAG_WRITABLE;
		break;
	case IOCTX_FLG_RDWR:
		fd->flags |= FO_FLAG_READABLE | FO_FLAG_WRITABLE;
		break;
	}
	if (opt & IOCTX_FLG_CLOEXEC)
	{
		fd->flags |= FO_FLAG_CLOEXEC;
	}

	// Here the vnode can choose not to support this, so we see ERROR_UNSUPPORTED as normal.
	err = node->open(fd, opt);
	if (err != ERROR_SUCCESS && err != ERROR_UNSUPPORTED)
	{
		return err;
	}

	fd->vnode->increase_open_count();

	return ERROR_SUCCESS;
}

error_code vfs_io_context::create_at(vnode_base* at, const char* full_path, mode_type mode)
{
	if (full_path == nullptr)
	{
		return -ERROR_INVALID;
	}

	char* path = new(std::nothrow)char[VFS_MAX_PATH_LEN];
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

error_code vfs_io_context::open_at(file_object* fd, vnode_base* at, const char* path, size_t flags, size_t mode)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

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

error_code vfs_io_context::close(file_object* fd)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	// TODO: socket can't be close

	if (fd->ref == 0)
	{
		return -ERROR_INVALID;
	}

	if (fd->vnode == nullptr)
	{
		return -ERROR_INVALID;
	}

	fd->vnode->decrease_open_count();

	auto ret = fd->vnode->close(fd);

	if (ret != ERROR_SUCCESS && ret != ERROR_UNSUPPORTED)
	{
		return ret;
	}

	return ERROR_SUCCESS;
}

error_code_with_result<size_t> vfs_io_context::read_directory(file_object* fd, directory_entry* ent)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	//TODO: socket?

	if ((fd->flags & FO_FLAG_DIRECTORY) == 0)
	{
		return -ERROR_NOT_DIR;
	}

	if (fd->flags & FO_FLAG_PSEUDO_DIR)
	{
		if (fd->flags & FO_FLAG_PSEUDO_DIR_DOT)
		{
			auto vnode = fd->vnode;

			if (vnode == nullptr)
			{
				return -ERROR_INVALID;
			}

			fd->flags &= ~FO_FLAG_PSEUDO_DIR_DOT;
			fd->flags |= FO_FLAG_PSEUDO_DIR_DOTDOT;

			ent->ino = vnode->get_inode_id();
			ent->type = DT_DIR;
			ent->off = 0;
			strncpy(ent->name, ".", 2);

			ent->reclen = sizeof(directory_entry) + 1;

			return (size_t)ent->reclen;
		}

		vnode_base* item = (vnode_base*)fd->pos;

		if (item == nullptr)
		{
			return -ERROR_INVALID;
		}

		// Fill directory entry
		ent->ino = item->get_inode_id();
		ent->off = 0;

		switch (item->get_type())
		{
		case vnode_types::VNT_REG:
			ent->type = DT_REG;
			break;
		case vnode_types::VNT_DIR:
		case vnode_types::VNT_MNT:
			ent->type = DT_DIR;
			break;
		default:
			ent->type = DT_UNKNOWN;
			break;
		}

		strncpy(ent->name, item->get_name(), VFS_MAX_PATH_LEN);
		ent->reclen = sizeof(directory_entry) + strnlen(item->get_name(), VFS_MAX_PATH_LEN);

		fd->pos = (size_t)item->get_next();

		return (size_t)ent->reclen;
	}
	else
	{
		return fd->vnode->read_directory(fd, ent);
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code vfs_io_context::unlink_at(vnode_base* at, const char* pathname, size_t flags)
{
	if (at == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto find_ret = find(at, pathname, true);
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}

	auto node = get_result(find_ret);

	if (node->get_type() == vnode_types::VNT_MNT)
	{
		return -ERROR_BUSY;
	}

	if (node->get_open_count() != 0)
	{
		return -ERROR_BUSY;
	}

	if (flags & AT_REMOVEDIR)
	{
		if (node->get_type() != vnode_types::VNT_DIR)
		{
			return -ERROR_NOT_DIR;
		}
		// TODO: empty check
	}
	else
	{
		if (node->get_type() == vnode_types::VNT_DIR)
		{
			return ERROR_IS_DIR;
		}
	}

	auto unlink_ret = node->unlink();
	if (unlink_ret != ERROR_SUCCESS)
	{
		return unlink_ret;
	}

	node->get_parent()->detach(node);

	// TODO: slab? freelist?
	delete node;

	return ERROR_SUCCESS;
}

error_code vfs_io_context::make_directory_at(vnode_base* vnode, const char* path, mode_type mode)
{
	if (vnode == nullptr)
	{
		vnode = this->cwd_vnode;
	}

	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto parent_name = new(std::nothrow)char[VFS_MAX_PATH_LEN];
	auto filename = next_path_element(path, parent_name);
	if (filename[0] == '\0')
	{
		delete[] parent_name;
		return -ERROR_NO_ENTRY;
	}

	auto find_ret = find(vnode, parent_name, false);
	if (has_error(find_ret))
	{
		delete[] parent_name;
		return get_error_code(find_ret);
	}
	vnode = get_result(find_ret);

	auto access_ret = access_node(vnode, W_OK);
	if (access_ret != ERROR_SUCCESS)
	{
		delete[] parent_name;
		return access_ret;
	}

	auto mkdir_ret = vnode->make_directory(filename, this->uid, this->gid, mode & ~this->mode_mask);

	delete[] parent_name;
	return mkdir_ret;
}

error_code_with_result<vnode_base*> vfs_io_context::make_node(const char* path, mode_type mode)
{
	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto parent_name = new(std::nothrow)char[VFS_MAX_PATH_LEN];
	auto file_name = next_path_element(path, parent_name);

	if (file_name[0] == '\0')
	{
		return -ERROR_NO_ENTRY;
	}

	auto find_ret = find(cwd_vnode, parent_name, false);
	if (has_error(find_ret))
	{
		delete[] parent_name;
		return get_error_code(find_ret);
	}

	auto vnode = get_result(find_ret);

	auto acc_ret = access_node(vnode, W_OK);
	if (acc_ret != ERROR_SUCCESS)
	{
		delete[] parent_name;
		return acc_ret;
	}

	vnode_types type = vnode_types::VNT_FIFO;
	switch (mode & S_IFMT)
	{
	case S_IFIFO:
		type = vnode_types::VNT_FIFO;
		break;
	case S_IFSOCK:
		type = vnode_types::VNT_SOCK;
		break;
	default:
		delete[] parent_name;
		return -ERROR_INVALID;
	}

	auto new_node_ret = vnode->allocate_new(file_name, gid, uid, mode & VFS_MODE_MASK & ~mode_mask);
	if (has_error(new_node_ret))
	{
		return get_error_code(new_node_ret);
	}

	auto new_node = get_result(new_node_ret);

	return new_node;
}

error_code vfs_io_context::change_mode(const char* path, mode_type mode)
{
	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	mode &= VFS_MODE_MASK;

	auto find_ret = find(cwd_vnode, path, true);
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}

	auto node = get_result(find_ret);

	if (node->get_type() == vnode_types::VNT_LNK)
	{
		return -ERROR_INVALID;
	}

	if (uid != UID_ROOT && uid != node->get_uid())
	{
		return -ERROR_ACCESS;
	}

	// TODO: the vnodes should have different implementation from the template
	auto err = node->change_mode(mode);
	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	return ERROR_SUCCESS;
}

error_code vfs_io_context::change_ownership(const char* path, uid_type uid, gid_type gid)
{
	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto find_ret = find(cwd_vnode, path, true);
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}

	auto node = get_result(find_ret);

	if (node->get_type() == vnode_types::VNT_LNK)
	{
		return -ERROR_INVALID;
	}

	if (uid != UID_ROOT)
	{
		return -ERROR_ACCESS;
	}

	auto err = node->change_ownership(uid, gid);
	if (err != ERROR_SUCCESS && err != ERROR_UNSUPPORTED)
	{
		return err;
	}
	else if (err == ERROR_UNSUPPORTED && node->has_flags(VNF_MEMORY) == false)
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

error_code vfs_io_context::ioctl(file_object* fd, size_t cmd, void* arg)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	// TODO: socket check

	auto vnode = fd->vnode;
	if (vnode == nullptr)
	{
		return -ERROR_INVALID;
	}

	return vnode->get_dev()->ioctl(cmd, arg);
}

error_code vfs_io_context::file_truncate(vnode_base* node, size_t size)
{
	if (node == nullptr)
	{
		return -ERROR_INVALID;
	}

	return node->truncate(size);
}

error_code vfs_io_context::file_access_at(vnode_base* at, const char* path, vfs_access_status access_mode, size_t flags)
{
	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	auto find_ret = find(at, path, flags); // FIXME: ?
	if (has_error(find_ret))
	{
		return get_error_code(find_ret);
	}
	auto node = get_result(find_ret);

	if (access_mode == F_OK)
	{
		return ERROR_SUCCESS;
	}

	return access_node(node, access_mode);
}

error_code vfs_io_context::file_status_at(vnode_base* at, const char* path, OUT file_status* st, size_t flags)
{
	if (at == nullptr)
	{
		at = cwd_vnode;
	}

	if (st == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (path == nullptr)
	{
		return -ERROR_INVALID;
	}

	vnode_base* node = at;
	if (flags & AT_EMPTY_PATH)
	{
		if (node == nullptr)
		{
			//TODO:
			KDEBUG_NOT_IMPLEMENTED;
		}
	}
	else
	{
		if (auto ret = find(at, path, flags & AT_SYMLINK_NOFOLLOW);!has_error(ret))
		{
			node = get_result(ret);
		}
	}

	return node->stat(st);
}

error_code vfs_io_context::access_check(int desm, mode_type mode, uid_type uid, gid_type gid)
{
	if (this->uid == 0)
	{
		if (desm & X_OK)
		{
			// Check if anyone at all can execute this
			if (!(mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
			{
				return -ERROR_ACCESS;
			}
		}

		return 0;
	}

	if (uid == this->uid)
	{
		if ((desm & R_OK) && !(mode & S_IRUSR))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & W_OK) && !(mode & S_IWUSR))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & X_OK) && !(mode & S_IXUSR))
		{
			return -ERROR_ACCESS;
		}
	}
	else if (gid == this->gid)
	{
		if ((desm & R_OK) && !(mode & S_IRGRP))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & W_OK) && !(mode & S_IWGRP))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & X_OK) && !(mode & S_IXGRP))
		{
			return -ERROR_ACCESS;
		}
	}
	else
	{
		if ((desm & R_OK) && !(mode & S_IROTH))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & W_OK) && !(mode & S_IWOTH))
		{
			return -ERROR_ACCESS;
		}
		if ((desm & X_OK) && !(mode & S_IXOTH))
		{
			return -ERROR_ACCESS;
		}
	}

	return ERROR_SUCCESS;
}

error_code vfs_io_context::access_node(vnode_base* vn, size_t mode)
{
	return access_check(mode, vn->get_mode(), this->uid, this->gid);
}

error_code_with_result<size_t> vfs_io_context::write(file_object* fd, const void* buf, size_t count)
{
	//TODO: socket checking
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (buf == nullptr)
	{
		return -ERROR_INVALID;
	}

	if ((!(fd->flags & FO_FLAG_WRITABLE)))
	{
		return -ERROR_ACCESS;
	}

	if (fd->flags & FO_FLAG_DIRECTORY)
	{
		return -ERROR_IS_DIR;
	}

	auto node = fd->vnode;

	if (node == nullptr)
	{
		return -ERROR_INVALID;
	}

	switch (node->get_type())
	{
	case vnode_types::VNT_FIFO:
	case vnode_types::VNT_REG:
		return fd->vnode->write(fd, buf, count);
		break;
	case vnode_types::VNT_BLK:
	case vnode_types::VNT_CHR:
		return fd->vnode->get_dev()->write(buf, fd->pos, count);
		break;
	default:
	case vnode_types::VNT_DIR:
		return -ERROR_INVALID;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code_with_result<size_t> vfs_io_context::read(file_object* fd, void* buf, size_t count)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (!(fd->flags & FO_FLAG_READABLE))
	{
		return -ERROR_INVALID;
	}

	if (buf == nullptr)
	{
		return -ERROR_INVALID;
	}

	switch (fd->vnode->get_type())
	{
	case vnode_types::VNT_FIFO:
	case vnode_types::VNT_REG:
		return fd->vnode->read(fd, buf, count);
		break;
	case vnode_types::VNT_BLK:
	case vnode_types::VNT_CHR:
		return fd->vnode->get_dev()->read(buf, fd->pos, count);
		break;
	default:
	case vnode_types::VNT_DIR:
		return -ERROR_INVALID;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code_with_result<size_t> vfs_io_context::seek(file_object* fd, size_t offset, vfs_seek_methods whence)
{
	if (fd == nullptr)
	{
		return -ERROR_INVALID;
	}

	// TODO: socket

	auto vnode = fd->vnode;
	if (vnode == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (vnode->get_type() != vnode_types::VNT_REG)
	{
		return -ERROR_NOT_FILE;
	}

	if (auto ret = vnode->seek(fd, offset, whence);has_error(ret))
	{
		if (auto err = get_error_code(ret);err == -ERROR_UNSUPPORTED)
		{
			return -ERROR_INVALID;
		}
		else
		{
			return err;
		}
	}
	else
	{
		return ret;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}


