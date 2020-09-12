#pragma once
#include "data/List.h"
#include "fs/device/dev.hpp"

namespace file_system
{

	enum vnode_flags
	{
		// Means the node has no physical storage and resides only in memory
		VNF_MEMORY = (1 << 0),

		// Means the link has different meanings depending on resolving process ID - use target_func instead
		VNF_PER_PROCESS = (1 << 1)
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

	enum mode_values : mode_type
	{
		S_IFMT = 0170000,
		S_IFSOCK = 0140000,
		S_IFLNK = 0120000,
		S_IFREG = 0100000,
		S_IFBLK = 0060000,
		S_IFDIR = 0040000,
		S_IFCHR = 0020000,
		S_IFIFO = 0010000,
	};

	enum block_device_type
	{
		DBT_SDx = 1,
		DBT_HDx = 2,
		DBT_RAM = 3,
		DBT_CDx = 4,
		DBT_PARTITION = 127,
		DBT_PSEUDO = 128,
		DBT_OTHER = 255,
	};

	enum char_device_type
	{
		CDT_TTY = 1
	};

	enum device_features : size_t
	{
		DFE_HAS_PARTITIONS = 1u << 1u,
		DFE_HAS_MEMMAP = 1u << 2u,
		DEF_HAS_CHILD_DEVICES = 1u << 3u
	};

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

	struct vfs_status
	{
		uint64_t bsize;
		uint64_t frsize;
		size_t blocks;
		size_t bfree;
		size_t bavail;
		size_t files;
		size_t ffree;
		size_t favail;
		uint64_t fsid;
		uint64_t flag;
		uint64_t namemax;
	};

	constexpr size_t VFS_MODE_MASK = 0xFFF;

	enum fs_class_id
	{
		FSC_EXT2,
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wunused-parameter"

	class fs_class_base;
	struct fs_instance;

	class fs_class_base
	{
	 private:
		static constexpr size_t FILE_SYSTEM_CLASS_NAME_MAX = 64;

	 private:
		char name[FILE_SYSTEM_CLASS_NAME_MAX]{};
		size_t opt{};
		fs_class_id id{};

	 public:
		const char* get_name() const
		{
			return name;
		}
		fs_class_id get_id() const
		{
			return id;
		}
	 public:
		list_head link{};

		~fs_class_base() = default;

		virtual vnode_base* get_root(fs_instance* fs) = 0;
		virtual error_code initialize(fs_instance* fs, const char* data) = 0;
		virtual void destroy(fs_instance* fs) = 0;
		virtual error_code statvfs(fs_instance* fs, OUT vfs_status* ret) = 0;

		virtual error_code register_this();
	};

	struct fs_instance
	{
		device_class* dev;
		fs_class_base* fs_class;

		size_t flags;
		void* private_data;

		list_head link;
	};

#pragma GCC diagnostic pop

	static inline constexpr mode_type vnode_type_to_mode_type(vnode_type type)
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

	class vnode_base
	{
	 protected:
		using vnode_link_getter_type = vnode_base* (*)(struct thread*, struct vnode*);

	 protected:
		static constexpr size_t VNODE_NAME_MAX = 64;

		vnode_type type;

		size_t flags{};

	 protected:

		size_t open_count{};
		size_t inode_id{};

		mode_type mode{};

		gid_type gid{};
		uid_type uid{};

		fs_instance* fs{};
		device_class* dev{};
		void* private_data{};

		char name_buf[VNODE_NAME_MAX]{};

		list_head child_head{};
		list_head link{};

		union
		{
			vnode_base* node;
			vnode_link_getter_type link_getter;
		};

	 public:
		virtual ~vnode_base() = default;

		vnode_base(vnode_type t, const char* n)
			: type(t)
		{
			if (n != nullptr)
			{
				strncpy(name_buf, n, strnlen(n, VNODE_NAME_MAX));
			}
			libkernel::list_init(&child_head);
		}

	 public:

		[[nodiscard]] const char* get_name() const
		{
			return name_buf;
		}

	 public:
		virtual error_code find(const char* name, vnode_base& ret) = 0;
		virtual size_t read_dir(const file_object& fd, directory_entry& entry) = 0;
		virtual error_code open_dir(const file_object& fd) = 0;
		virtual error_code open(const file_object& fd) = 0;
		virtual error_code close(const file_object& fd) = 0;

		virtual error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code truncate(size_t size) = 0;
		virtual error_code unlink(vnode_base& vn) = 0;

		virtual uintptr_t lseek(const file_object& fd, size_t offset, int whence) = 0;

		virtual error_code stat(file_status& st) = 0;
		virtual error_code chmod(size_t mode) = 0;
		virtual error_code chown(uid_type uid, gid_type gid) = 0;

		virtual error_code read_link(char* buf, size_t lim) = 0;

		virtual size_t read(const file_object& fd, void* buf, size_t count) = 0;
		virtual size_t write(const file_object& fd, const void* buf, size_t count) = 0;

	 public:
		friend error_code init_devfs_root();
		friend error_code device_add(device_class_id cls, size_t subcls, device_class& dev, const char* name);
		friend error_code partition_add_device(file_system::vnode_base& parent,
			logical_block_address lba,
			size_t size,
			size_t disk_idx,
			[[maybe_unused]]uint32_t sys_id);
	};

	class dev_fs_node
		: public vnode_base
	{
	 public:
		dev_fs_node(vnode_type t, const char* n)
			: vnode_base(t, n)
		{
		}

		~dev_fs_node() override = default;

		error_code find(const char* name, vnode_base& ret) override;
		size_t read_dir(const file_object& fd, directory_entry& entry) override;
		error_code open_dir(const file_object& fd) override;
		error_code open(const file_object& fd) override;
		error_code close(const file_object& fd) override;
		error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code truncate(size_t size) override;
		error_code unlink(vnode_base& vn) override;
		uintptr_t lseek(const file_object& fd, size_t offset, int whence) override;
		error_code stat(file_status& st) override;
		error_code chmod(size_t mode) override;
		error_code chown(uid_type uid, gid_type gid) override;
		error_code read_link(char* buf, size_t lim) override;
		size_t read(const file_object& fd, void* buf, size_t count) override;
		size_t write(const file_object& fd, const void* buf, size_t count) override;

	};

	error_code init_devfs_root();
}