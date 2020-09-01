#pragma once
#include "system/types.h"
#include <cstring>

namespace file_system
{
	struct directory_entry
	{
		uint64_t ino;
		uintptr_t off;
		uint16_t reclen;
		uint8_t type;
		char name[0];
	};

	struct file_object
	{
		size_t flags;
		size_t ref;
		size_t pos;

		class IVNode* vnode;
		void* private_data;
	};

	struct file_status
	{
		uint32_t dev;
		uint32_t ino;
		size_t mode;
		uint32_t nlink;
		uid_type uid;
		gid_type gid;
		uint32_t rdev;
		uint32_t size;
		uint32_t blksize;
		uint32_t blocks;
		size_t atime;
		size_t mtime;
		size_t ctime;
	};

	class IDevice
	{
	 protected:
		void* dev_data;
		size_t block_size;
		size_t flags;
	 public:
		virtual ~IDevice()
		{
		};

		virtual size_t read(void* buf, uintptr_t offset, size_t count) = 0;
		virtual size_t write(const void* buf, uintptr_t offset, size_t count) = 0;
		virtual error_code ioctl(size_t req, void* args) = 0;
	};

	class IMemmap
	{
	 public:
		virtual ~IMemmap()
		{
		};
		virtual error_code mmap(uintptr_t base, size_t page_count, int prot, size_t flags) = 0;
	};

	enum vnode_type
	{
		VNT_REG,
		VNT_DIR,
		VNT_BLK,
		VNT_CHR,
		VNT_LNK,
		VNT_FIFO,
		VNT_SOCK,
		VNT_UNK,
		VNT_MNT,
	};

	class IVNode
	{
	 private:
		using vnode_link_getter_type = IVNode* (*)(struct thread*, struct vnode*);

	 private:
		static constexpr size_t VNODE_NAME_MAX = 64;

		vnode_type type;

		char name_buf[VNODE_NAME_MAX]{};

		list_head child_head{};

		union
		{
			IVNode* node{};
			vnode_link_getter_type link_getter;
		};
	 public:
		virtual ~IVNode() = default;

		IVNode(vnode_type t, const char* n)
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
			IVNode::type = type;
		}

	 public:
		virtual error_code find(const char* name, IVNode& ret) = 0;
		virtual size_t read_dir(const file_object& fd, directory_entry& entry) = 0;
		virtual error_code open_dir(const file_object& fd) = 0;
		virtual error_code open(const file_object& fd) = 0;
		virtual error_code close(const file_object& fd) = 0;

		virtual error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code truncate(size_t size) = 0;
		virtual error_code unlink(IVNode& vn) = 0;

		virtual uintptr_t lseek(const file_object& fd, size_t offset, int whence) = 0;

		virtual error_code stat(file_status& st) = 0;
		virtual error_code chmod(size_t mode) = 0;
		virtual error_code chown(uid_type uid, gid_type gid) = 0;

		virtual error_code read_link(char* buf, size_t lim) = 0;

		virtual size_t read(const file_object& fd, void* buf, size_t count) = 0;
		virtual size_t write(const file_object& fd, const void* buf, size_t count) = 0;

	};

	PANIC void fs_init();
}