#pragma once

#include "system/error.hpp"

#include "drivers/debug/kdebug.h"

#include "data/pod_list.h"
#include "data/list_base.hpp"

#include "fs/device/device.hpp"

namespace file_system
{

	enum vnode_flags : uint64_t
	{
		// Means the node has no physical storage and resides only in memory
		VNF_MEMORY = (1u << 0u),

		// Means the link has different meanings depending on resolving process ID - use target_func instead
		VNF_PER_PROCESS = (1u << 1u)
	};

	enum class vnode_types : uint64_t
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

	enum directory_entry_type
	{
		DT_UNKNOWN = 0,
		DT_FIFO = 1,
		DT_CHR = 2,
		DT_DIR = 4,
		DT_BLK = 6,
		DT_REG = 8,
		DT_LNK = 10,
		DT_SOCK = 12,
		DT_WHT = 14
	};

	struct directory_entry
	{
		uint64_t ino;
		uintptr_t off;
		uint16_t reclen;
		uint8_t type;
		char name[0];
	};

	enum file_object_flags
	{
		FO_FLAG_WRITABLE = (1 << 0),
		FO_FLAG_READABLE = (1 << 1),
		FO_FLAG_DIRECTORY = (1 << 2),
		FO_FLAG_PSEUDO_DIR = (1 << 3),
		FO_FLAG_PSEUDO_DIR_DOT = (1 << 4),
		FO_FLAG_PSEUDO_DIR_DOTDOT = (1 << 5),
		FO_FLAG_CLOEXEC = (1 << 7),
	};

	struct file_object
	{
		size_t flags;
		size_t ref;
		size_t pos;

		vnode_base* vnode;
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

	constexpr size_t VFS_MAX_PATH_LEN = 256;

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

		bool is_initialized = false;

	 public:

		virtual error_code register_this();

		[[nodiscard]] virtual const char* get_name() const
		{
			return name;
		}

		[[nodiscard]] virtual fs_class_id get_id() const
		{
			return id;
		}

		[[nodiscard]] virtual bool is_initialized1() const
		{
			return is_initialized;
		}

	 public:

		~fs_class_base() = default;

		MUST_SUPPORT virtual error_code_with_result<vnode_base*> get_root() = 0;
		MUST_SUPPORT virtual error_code_with_result<vfs_status> get_vfs_status(fs_instance* fs) = 0;

		MUST_SUPPORT virtual error_code initialize(fs_instance* fs, OPTIONAL const char* data) = 0;
		OPTIONAL_SUPPORT virtual error_code dispose(fs_instance* fs) = 0;
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

	static inline constexpr mode_type vnode_type_to_mode_type(vnode_types type)
	{
		switch (type)
		{
		case vnode_types::VNT_MNT:
		case vnode_types::VNT_DIR:
			return S_IFDIR;
		case vnode_types::VNT_LNK:
			return S_IFLNK;
		case vnode_types::VNT_BLK:
			return S_IFBLK;
		case vnode_types::VNT_CHR:
			return S_IFCHR;
		default:
		case vnode_types::VNT_REG:
			return S_IFREG;
		}

		// shouldn't reach here

		return S_IFREG;
	}

	class vnode_base
		: public libkernel::single_linked_list_base<vnode_base*>
	{
	 public:
		enum seek_methods : uint64_t
		{
			SM_FROM_START,
			SM_FROM_CURRENT,
			SM_FROM_END,
		};
	 protected:
		static constexpr size_t VNODE_NAME_MAX = 64;

		vnode_types type;

		size_t flags{};
		size_t open_count{};

		size_t inode_id{};

		mode_type mode{};

		gid_type gid{};
		uid_type uid{};

		fs_instance* fs{};

		device_class* dev{};

		void* private_data{};

		char name_buf[VNODE_NAME_MAX]{};

		vnode_base* parent;

		union
		{
			vnode_base* node_target;
			vnode_link_getter_type link_getter;
		} link_target;

	 public:
		virtual ~vnode_base() = default;

		vnode_base(vnode_types t, const char* n)
			: type(t), parent(nullptr)
		{
			if (n != nullptr)
			{
				strncpy(name_buf, n, strnlen(n, VNODE_NAME_MAX));
			}

			kdebug::kdebug_log("Create vnode: type %d, name %s\n", (uint32_t)t, n);
		}

	 public:

		vnode_base* get_parent() const
		{
			return parent;
		}

		void set_parent(vnode_base* parent)
		{
			vnode_base::parent = parent;
		}

		[[nodiscard]] vnode_base* get_link_target() const
		{
			if (this->type != vnode_types::VNT_MNT)
			{
				return nullptr;
			}
			return link_target.node_target;
		}

		void set_link_target(vnode_base* target)
		{
			link_target.node_target = target;
		}

		[[nodiscard]] vnode_link_getter_type get_link_getter_func() const
		{
			if (this->type != vnode_types::VNT_MNT)
			{
				return nullptr;
			}
			return link_target.link_getter;
		}

		void set_link_getter_func(vnode_link_getter_type getter)
		{
			link_target.link_getter = getter;
		}

		[[nodiscard]] const char* get_name() const
		{
			return name_buf;
		}

		[[nodiscard]] fs_instance* get_fs() const
		{
			return fs;
		}

		void set_fs(fs_instance* the_fs)
		{
			vnode_base::fs = the_fs;
		}

		[[nodiscard]] size_t get_flags() const
		{
			return flags;
		}

		[[nodiscard]] bool has_flags(size_t flgs) const
		{
			return flags & flgs;
		}

		void set_flags(size_t flgs)
		{
			vnode_base::flags = flgs;
		}

		[[nodiscard]]device_class* get_dev() const
		{
			return dev;
		}

		[[nodiscard]]vnode_types get_type() const
		{
			return type;
		}

		void set_type(vnode_types t)
		{
			vnode_base::type = t;
		}

		[[nodiscard]] size_t get_open_count() const
		{
			return open_count;
		}

		void set_open_count(size_t oc)
		{
			vnode_base::open_count = oc;
		}

		void increase_open_count()
		{
			vnode_base::open_count++;
		}

		void decrease_open_count()
		{
			vnode_base::open_count--;
		}

		[[nodiscard]]size_t get_inode_id() const
		{
			return inode_id;
		}

	 public:

		error_code attach(vnode_base* child);
		error_code detach(vnode_base* node);
		error_code_with_result<vnode_base*> lookup_child(const char* name);

	 public:
		virtual error_code_with_result<vnode_base*> find(const char* name) = 0;
		virtual size_t read_directory(const file_object* fd, directory_entry* entry) = 0;
		virtual error_code open_dir(const file_object* fd) = 0;
		virtual error_code open(const file_object* fd, mode_type opt) = 0;
		virtual error_code close(const file_object* fd) = 0;

		virtual error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) = 0;
		virtual error_code truncate(size_t size) = 0;
		virtual error_code unlink(vnode_base& vn) = 0;

		virtual error_code_with_result<offset_t> seek(file_object* fd, size_t offset, seek_methods whence) = 0;

		virtual error_code stat(file_status& st) = 0;
		virtual error_code chmod(size_t mode) = 0;
		virtual error_code chown(uid_type uid, gid_type gid) = 0;

		virtual error_code read_link(char* buf, size_t lim) = 0;

		virtual error_code_with_result<size_t> read(file_object* fd, void* buf, size_t count) = 0;
		virtual error_code_with_result<size_t> write(file_object* fd, const void* buf, size_t count) = 0;

	 public:

		friend error_code init_devfs_root();
		friend error_code partition_add_device(file_system::vnode_base& parent,
			logical_block_address lba,
			size_t size,
			size_t disk_idx,
			[[maybe_unused]]uint32_t sys_id);

		friend error_code device_add(device_class_id cls, size_t subcls, device_class& dev, const char* name);
		friend error_code_with_result<vnode_base*> device_find_first(device_class_id cls, const char* name);
		friend error_code device_add_link(const char* name, vnode_link_getter_type getter);
		friend error_code device_add_link(const char* name, vnode_base* to);
	};

	class dev_fs_node
		: public vnode_base
	{
	 public:
		dev_fs_node(vnode_types t, const char* n)
			: vnode_base(t, n)
		{
		}

		~dev_fs_node() override = default;

		error_code_with_result<vnode_base*> find(const char* name) override;
		size_t read_directory(const file_object* fd, directory_entry* entry) override;
		error_code open_dir(const file_object* fd) override;
		error_code open(const file_object* fd, mode_type opt) override;
		error_code close(const file_object* fd) override;
		error_code create(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode) override;
		error_code truncate(size_t size) override;
		error_code unlink(vnode_base& vn) override;
		error_code_with_result<offset_t> seek(file_object* fd, size_t offset, seek_methods whence) override;
		error_code stat(file_status& st) override;
		error_code chmod(size_t mode) override;
		error_code chown(uid_type uid, gid_type gid) override;
		error_code read_link(char* buf, size_t lim) override;
		error_code_with_result<size_t> read(file_object* fd, void* buf, size_t count) override;
		error_code_with_result<size_t> write(file_object* fd, const void* buf, size_t count) override;

	};

	constexpr size_t IOCTX_FLG_MASK_ACCESS_MODE = (3 << 0);

	enum vfs_ioctx_flags : mode_type
	{
		IOCTX_FLG_EXEC = (1 << 2),
		IOCTX_FLG_RDONLY = (0 << 0),
		IOCTX_FLG_WRONLY = (1 << 0),
		IOCTX_FLG_RDWR = (2 << 0),
		IOCTX_FLG_CREATE = (1 << 6),
		IOCTX_FLG_EXCL = (1 << 7),
		IOCTX_FLG_TRUNC = (1 << 9),
		IOCTX_FLG_APPEND = (1 << 10),
		IOCTX_FLG_DIRECTORY = (1 << 16),
		IOCTX_FLG_CLOEXEC = (1 << 19),
	};

	enum vfs_access_status : size_t
	{
		R_OK = 4,
		W_OK = 2,
		X_OK = 1,
		F_OK = 0,
	};

	class vfs_io_context
	{
	 private:
		static constexpr size_t LINK_MAX = 32;
	 private:
		vnode_base* cwd_vnode{};
		uid_type uid{};
		gid_type gid{};
		mode_type mode_mask{};

	 private:
		static const char* next_path_element(const char* path, OUT char* element);
		static error_code open_directory(file_object* fd);
		static error_code_with_result<vnode_base*> lookup_or_load_node(vnode_base* at, const char* name);
	 private:
		error_code_with_result<vnode_base*> do_find(vnode_base* at, const char* path, bool link_itself);
		error_code_with_result<const char*> separate_parent_name(const char* full_path, OUT char* path);
		error_code_with_result<vnode_base*> do_link_resolve(vnode_base* lnk, bool link_itself, size_t count);
	 public:
		vfs_io_context() = default;

		vfs_io_context(vnode_base* vnode, uid_type _uid, gid_type _gid)
			: cwd_vnode(vnode), uid(_uid), gid(_gid)
		{

		}
	 public:

		[[nodiscard]]error_code set_cwd(const char* path);
		[[nodiscard]]error_code vnode_get_path(char* path, vnode_base* node);

		[[nodiscard]]error_code_with_result<vnode_base*> link_resolve(vnode_base* lnk, bool link_itself);
		[[nodiscard]]error_code_with_result<vnode_base*> find(vnode_base* rel, const char* path, bool link_itself);
		[[nodiscard]]error_code mount(const char* at,
			device_class* blk,
			fs_class_id fs_id,
			size_t flags,
			const char* opt);
		[[nodiscard]]error_code umount(const char* dir_name);

		[[nodiscard]]error_code open_vnode(file_object* fd, vnode_base* node, mode_type opt);
		[[nodiscard]]error_code create_at(vnode_base* at, const char* path, mode_type mode);
		[[nodiscard]]error_code open_at(
			file_object* fd,
			vnode_base* at,
			const char* path,
			size_t flags, size_t mode);
		[[nodiscard]]error_code close(file_object* fd);
		[[nodiscard]]error_code_with_result<size_t> read_directory(file_object* fd, directory_entry* ent);
		[[nodiscard]]error_code unlink_at(vnode_base* at, const char* pathname, size_t flags);
		[[nodiscard]]error_code make_directory_at(vnode_base* at, const char* path, mode_type mode);
		[[nodiscard]]error_code_with_result<vnode_base*> make_node(const char* path, mode_type mode);
		[[nodiscard]]error_code change_mode(const char* path, mode_type mode);
		[[nodiscard]]error_code chown(const char* path, uid_type uid, gid_type gid);
		[[nodiscard]]error_code ioctl(file_object* fd, size_t cmd, void* arg);
		[[nodiscard]]error_code file_truncate(vnode_base* node, size_t length);

		[[nodiscard]]error_code file_access_at(vnode_base* at,
			const char* path,
			vfs_access_status access_mode,
			size_t flags);

		[[nodiscard]]error_code file_status_at(vnode_base* at, const char* path, file_status* st, size_t flags);
		[[nodiscard]]error_code access_check(int desm, mode_type mode, uid_type uid, gid_type gid);
		[[nodiscard]]error_code access_node(vnode_base* vn, size_t mode);

		[[nodiscard]]error_code_with_result<size_t> write(file_object* fd, const void* buf, size_t count);
		[[nodiscard]]error_code_with_result<size_t> read(file_object* fd, void* buf, size_t count);

		[[nodiscard]]size_t seek(file_object* fd, size_t offset, size_t whence);
	};

	extern vfs_io_context* const kernel_io_context;

	error_code vfs_init();
	error_code vnode_init();
}