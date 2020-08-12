#pragma once
#include "vfs.hpp"

namespace fs
{
	class Ext2FileSystem : public IFileSystem
	{
	 private:
		const char* name = "ext2fs";
		static constexpr size_t fs_id = FSID_EXT2;

	 public:
		size_t get_id() const override;
		const char * get_name() const override;
	};
}