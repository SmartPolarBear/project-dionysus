#pragma once

#include "fs/ext2/ext2.hpp"

#include "drivers/debug/kdebug.h"

#include "system/types.h"
#include "system/error.hpp"

namespace file_system
{
	class ext2_vnode
		: public vnode_base
	{
	 public:
		ext2_vnode(fs_instance* fs, vnode_types t, const char* name)
			: vnode_base(t, name)
		{
			set_fs(fs);
		}

		ext2_vnode(vnode_types t, const char* n)
			: vnode_base(t, n)
		{
		}

		~ext2_vnode() override = default;

		[[nodiscard]]error_code initialize_from_inode(ext2_ino_type ino, const ext2_inode* src);

		[[nodiscard]]error_code_with_result<vnode_base*> find(const char* name) override;
		[[nodiscard]]size_t read_directory(const file_object* fd, directory_entry* entry) override;
		[[nodiscard]]error_code open_dir(const file_object* fd) override;
		[[nodiscard]]error_code open(const file_object* fd, mode_type opt) override;
		[[nodiscard]]error_code close(const file_object* fd) override;
		[[nodiscard]]error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		[[nodiscard]]error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		[[nodiscard]]error_code truncate(size_t size) override;
		[[nodiscard]]error_code unlink(vnode_base& vn) override;
		[[nodiscard]]error_code_with_result<offset_t> seek(file_object* fd, size_t offset, seek_methods whence) override;
		[[nodiscard]]error_code stat(file_status& st) override;
		[[nodiscard]]error_code chmod(size_t mode) override;
		[[nodiscard]]error_code chown(uid_type uid, gid_type gid) override;
		[[nodiscard]]error_code read_link(char* buf, size_t lim) override;
		[[nodiscard]]error_code_with_result<size_t> read(file_object* fd, void* buf, size_t count) override;
		[[nodiscard]]error_code_with_result<size_t> write(file_object* fd, const void* buf, size_t count) override;
	};
}