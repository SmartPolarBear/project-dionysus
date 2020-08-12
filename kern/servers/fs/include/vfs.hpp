#pragma once
#include "system/types.h"

namespace fs
{
	enum filesystem_id
	{
		FSID_EXT2
	};

	class IFileSystem
	{
	 public:
		virtual ~IFileSystem() = default;

		[[nodiscard]] virtual const char* get_name() const = 0;
		[[nodiscard]] virtual size_t get_id() const = 0;

	};
}