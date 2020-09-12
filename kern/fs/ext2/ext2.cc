#include "fs/ext2/ext2.hpp"

#include "fs/vfs/vfs.hpp"

using namespace file_system;

ext2_fs_class file_system::g_ext2fs;

file_system::vnode_base* file_system::ext2_fs_class::get_root(fs_instance* fs)
{
	return nullptr;
}

error_code file_system::ext2_fs_class::initialize(fs_instance* fs, const char* data)
{
	return 0;
}

void file_system::ext2_fs_class::destroy(fs_instance* fs)
{

}

error_code file_system::ext2_fs_class::statvfs(fs_instance* fs, vfs_status* ret)
{
	return 0;
}
