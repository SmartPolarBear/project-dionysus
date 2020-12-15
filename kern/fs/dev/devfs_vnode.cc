#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/kmem.hpp"

#include "drivers/cmos/rtc.hpp"
#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"

#include "fs/device/ata_devices.hpp"
#include "fs/device/device.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

error_code_with_result<file_system::vnode_base*> file_system::dev_fs_node::find(const char* name)
{
	return -ERROR_UNSUPPORTED;
}

size_t file_system::dev_fs_node::read_directory(const file_system::file_object* fd, file_system::directory_entry* entry)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::open_dir(const file_system::file_object* fd)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::open(const file_system::file_object* fd, mode_type opt)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::close(const file_system::file_object* fd)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::create(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::truncate(size_t size)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::unlink(file_system::vnode_base* vn)
{
	return -ERROR_UNSUPPORTED;
}

error_code_with_result<offset_t> file_system::dev_fs_node::seek(file_system::file_object* fd,
	size_t offset,
	vfs_seek_methods whence)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::chmod(size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::chown(uid_type uid, gid_type gid)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::dev_fs_node::read_link(char* buf, size_t lim)
{
	return -ERROR_UNSUPPORTED;
}

error_code_with_result<size_t> file_system::dev_fs_node::read(file_system::file_object* fd, void* buf, size_t count)
{
	return -ERROR_UNSUPPORTED;
}

error_code_with_result<size_t> file_system::dev_fs_node::write(file_system::file_object* fd,
	const void* buf,
	size_t count)
{
	return -ERROR_UNSUPPORTED;
}

#pragma clang diagnostic pop

error_code file_system::dev_fs_node::stat(file_system::file_status& st)
{
	st.mode = (this->mode & VFS_MODE_MASK) | vnode_type_to_mode_type(this->type);
	st.uid = this->uid;
	st.gid = this->gid;
	st.size = 0;
	st.blksize = 0;
	st.blocks = 0;
	st.ino = 0;

	st.atime = cmos::get_boot_timestamp();
	st.mtime = cmos::get_boot_timestamp();
	st.ctime = cmos::get_boot_timestamp();

	st.dev = 0;
	st.rdev = 0;

	return ERROR_SUCCESS;
}

error_code_with_result<file_system::vnode_base*> file_system::dev_fs_node::allocate_new(const char* name,
	gid_type gid,
	uid_type uid,
	mode_type mode)
{
	return 0;
}



