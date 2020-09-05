#pragma once
#include "fs/fs.hpp"

#include "data/List.h"

namespace file_system
{

	struct FileSystemInstance;
	class VNodeBase;
	class IDevice;

	enum dev_class
	{
		DEV_CLASS_BLOCK = 1,
		DEV_CLASS_CHAR = 2,
		DEV_CLASS_ANY = 255
	};

	enum block_device_type
	{
		DEV_BLOCK_SDx = 1,
		DEV_BLOCK_HDx = 2,
		DEV_BLOCK_RAM = 3,
		DEV_BLOCK_CDx = 4,
		DEV_BLOCK_PART = 127,
		DEV_BLOCK_PSEUDO = 128,
		DEV_BLOCK_OTHER = 255,
	};

	enum char_device_type
	{
		DEV_CHAR_TTY = 1
	};

	constexpr size_t VFS_MODE_MASK = 0xFFF;

	static inline constexpr mode_type vnode_type_to_mode_type(enum vnode_type type)
	{
		switch (type)
		{
		case VNT_MNT:
		case VNT_DIR:
			return S_IFDIR;
		case VNT_LNK:
			return S_IFLNK;
		case VNT_BLK:
			return S_IFBLK;
		case VNT_CHR:
			return S_IFCHR;
		default:
		case VNT_REG:
			return S_IFREG;
		}

		// shouldn't reach here
		return S_IFREG;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wunused-parameter"

	class FileSystemClassBase
	{
	 private:
		static constexpr size_t FILE_SYSTEM_CLASS_NAME_MAX = 64;

	 private:
		char name[FILE_SYSTEM_CLASS_NAME_MAX]{};
		size_t opt{};

	 public:
		list_head link{};

		FileSystemClassBase(FileSystemInstance& fs, const char* opt)
		{
		}

		~FileSystemClassBase() = default;

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

#pragma GCC diagnostic pop

	enum vnode_flags
	{
		// Means the node has no physical storage and resides only in memory
		VNF_MEMORY = (1 << 0),

		// Means the link has different meanings depending on resolving process ID - use target_func instead
		VNF_PER_PROCESS = (1 << 1)
	};

	class VNodeBase
	{
	 protected:
		using vnode_link_getter_type = VNodeBase* (*)(struct thread*, struct vnode*);

	 protected:
		static constexpr size_t VNODE_NAME_MAX = 64;

		vnode_type type;

		size_t flags{};

	 protected:

		size_t open_count{};
		size_t ino{};

		mode_type mode{};

		gid_type gid{};
		uid_type uid{};

		FileSystemInstance* fs{};
		IDevice* dev{};
		void* private_data{};

		char name_buf[VNODE_NAME_MAX]{};

		list_head child_head{};
		list_head link{};

		union
		{
			VNodeBase* node;
			vnode_link_getter_type link_getter;
		};

	 public:
		virtual ~VNodeBase() = default;

		VNodeBase(vnode_type t, const char* n)
			: type(t)
		{
			if (n != nullptr)
			{
				strncpy(name_buf, n, strnlen(n, VNODE_NAME_MAX));
			}
			libkernel::list_init(&child_head);
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

		size_t GetFlags() const
		{
			return flags;
		}

		void SetFlags(size_t flags)
		{
			VNodeBase::flags = flags;
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

	 public:
		friend error_code devfs_create_root_if_not_exist();
		friend error_code device_add(dev_class cls, size_t subcls, IDevice& dev, const char* name);
	};
}