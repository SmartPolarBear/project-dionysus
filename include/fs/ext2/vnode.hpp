#pragma once

#include "fs/ext2/ext2.hpp"

#include "system/types.h"
#include "system/error.hpp"

namespace file_system
{
	class ext2_vnode
		: public vnode_base
	{
	 public:
		ext2_vnode(vnode_type t, const char* n)
			: vnode_base(t, n)
		{
		}

		~ext2_vnode() override = default;

		error_code initialize_from_inode(ext2_ino_type ino, const ext2_inode* src);

		error_code_with_result<vnode_base*> find(const char* name) override;
		size_t read_dir(const file_object& fd, directory_entry& entry) override;
		error_code open_dir(const file_object& fd) override;
		error_code open(const file_object& fd) override;
		error_code close(const file_object& fd) override;
		error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code truncate(size_t size) override;
		error_code unlink(vnode_base& vn) override;
		error_code_with_result<offset_t> seek(file_object& fd, size_t offset, seek_methods whence) override;
		error_code stat(file_status& st) override;
		error_code chmod(size_t mode) override;
		error_code chown(uid_type uid, gid_type gid) override;
		error_code read_link(char* buf, size_t lim) override;
		error_code_with_result<size_t> read(file_object& fd, void* buf, size_t count) override;
		error_code_with_result<size_t> write(file_object& fd, const void* buf, size_t count) override;
	};
}