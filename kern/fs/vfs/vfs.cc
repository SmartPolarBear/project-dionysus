#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"

using namespace file_system;

error_code fs_class_base::register_this()
{
	return fs_register(this);
}
