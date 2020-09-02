#pragma once
#include "fs/vfs/vfs.hpp"

namespace file_system
{
	class Dev	FSVNode
		: public VNodeBase
	{
	 public:
		DEVFSVNode(vnode_type t, const char* n)
			: VNodeBase(t, n)
		{
		}



	};
}