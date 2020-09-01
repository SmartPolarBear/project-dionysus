#pragma once
#include "fs/fs.hpp"
#include "fs/device/dev.hpp"

namespace file_system
{
	class VNodeBase
	{
	 private:
		using vnode_link_getter_type = VNodeBase* (*)(struct thread*, struct vnode*);

	 private:
		static constexpr size_t VNODE_NAME_MAX = 64;

		vnode_type type;

		char name_buf[VNODE_NAME_MAX]{};

		list_head child_head{};

		union
		{
			VNodeBase* node{};
			vnode_link_getter_type link_getter;
		};
	 public:
		virtual ~VNodeBase() = default;

		VNodeBase(vnode_type t, const char* n)
			: type(t)
		{
			strncpy(name_buf, n, strnlen(n, VNODE_NAME_MAX));
		}

	 public:

		const char* GetName() const
		{
			return name_buf;
		}

		vnode_type GetType() const
		{
			return type;
		}

		void SetType(vnode_type type)
		{
			VNodeBase::type = type;
		}

	 public:
		virtual error_code find(const char* name, VNodeBase& ret) = 0;
		virtual size_t read_dir(const file_object& fd, directory_entry& entry) = 0;
		virtual error_code open_dir(const file_object& fd) = 0;
		virtual error_code open(const file_object& fd) = 0;
		virtual error_code close(const file_object& fd) = 0;

		virtual error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code truncate(size_t size) = 0;
		virtual error_code unlink(VNodeBase& vn) = 0;

		virtual uintptr_t lseek(const file_object& fd, size_t offset, int whence) = 0;

		virtual error_code stat(file_status& st) = 0;
		virtual error_code chmod(size_t mode) = 0;
		virtual error_code chown(uid_type uid, gid_type gid) = 0;

		virtual error_code read_link(char* buf, size_t lim) = 0;

		virtual size_t read(const file_object& fd, void* buf, size_t count) = 0;
		virtual size_t write(const file_object& fd, const void* buf, size_t count) = 0;

	};

	struct  FileSystemInstance;

	class FileSystemClassBase
	{
	 private:
		static constexpr size_t FILE_SYSTEM_CLASS_NAME_MAX = 64;

	 private:
		char name[FILE_SYSTEM_CLASS_NAME_MAX];
		size_t opt;

	 public:
		list_head link;

		FileSystemClassBase(FileSystemInstance& fs, const char* opt)
		{
		}

		~FileSystemClassBase()
		{
		}

		virtual VNodeBase* get_root(FileSystemInstance& fs) = 0;
	};

	struct FileSystemInstance
	{
		IDevice* dev;
		FileSystemClassBase* fs_class;

		size_t flags;
		void* private_data;

		list_head link;
	};
}