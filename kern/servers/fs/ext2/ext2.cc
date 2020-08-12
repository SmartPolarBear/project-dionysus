#include "ext2.hpp"

size_t fs::Ext2FileSystem::get_id() const
{
	return this->fs_id;
}

const char* fs::Ext2FileSystem::get_name() const
{
	return this->name;
}
