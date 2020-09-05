#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"

#include "fs/device/ATABlockDevice.hpp"
#include "fs/device/dev.hpp"
#include "fs/vfs/vnode.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

error_code file_system::DevFSVNode::find(const char* name, file_system::VNodeBase& ret)
{
	return -ERROR_UNSUPPORTED;
}

size_t file_system::DevFSVNode::read_dir(const file_system::file_object& fd, file_system::directory_entry& entry)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::open_dir(const file_system::file_object& fd)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::open(const file_system::file_object& fd)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::close(const file_system::file_object& fd)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::create(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::make_dir(const char* filename, uid_type uid, gid_type gid, size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::truncate(size_t size)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::unlink(file_system::VNodeBase& vn)
{
	return -ERROR_UNSUPPORTED;
}

uintptr_t file_system::DevFSVNode::lseek(const file_system::file_object& fd, size_t offset, int whence)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::chmod(size_t mode)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::chown(uid_type uid, gid_type gid)
{
	return -ERROR_UNSUPPORTED;
}

error_code file_system::DevFSVNode::read_link(char* buf, size_t lim)
{
	return -ERROR_UNSUPPORTED;
}
size_t file_system::DevFSVNode::read(const file_system::file_object& fd, void* buf, size_t count)
{
	return -ERROR_UNSUPPORTED;
}

size_t file_system::DevFSVNode::write(const file_system::file_object& fd, const void* buf, size_t count)
{
	return -ERROR_UNSUPPORTED;
}

#pragma clang diagnostic pop

error_code file_system::DevFSVNode::stat(file_system::file_status& st)
{
	st.mode = (this->mode & VFS_MODE_MASK) | vnode_type_to_mode_type(this->type);
	st.uid = this->uid;
	st.gid = this->gid;
	st.size = 0;
	st.blksize = 0;
	st.blocks = 0;
	st.ino = 0;

	// TODO: these all should be the boot time
	st.atime = 0;
	st.mtime = 0;
	st.ctime = 0;

	st.dev = 0;
	st.rdev = 0;

	return ERROR_SUCCESS;
}



