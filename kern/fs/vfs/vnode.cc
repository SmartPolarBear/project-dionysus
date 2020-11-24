#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/kmem.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"

#include "fs/device/ata_devices.hpp"
#include "fs/device/device.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>

//struct vnode_child_node
//{
//	file_system::vnode_base* vnode;
//	list_head list;
//};

using namespace memory::kmem;
using namespace libkernel;

//memory::kmem::kmem_cache* vnode_child_node_cache = nullptr;

error_code file_system::vnode_init()
{
//	vnode_child_node_cache = kmem_cache_create("vnode_child", sizeof(vnode_child_node));
//
//	if (vnode_child_node_cache == nullptr)
//	{
//		return -ERROR_MEMORY_ALLOC;
//	}

	return ERROR_SUCCESS;
}

error_code file_system::vnode_base::attach(file_system::vnode_base* child)
{
	return this->add_node(child);
}

error_code file_system::vnode_base::detach(file_system::vnode_base* node)
{
	node->parent = nullptr;
	return this->remove_node(node);
}

error_code_with_result<file_system::vnode_base*> file_system::vnode_base::lookup_child(const char* name)
{
	if (strnlen(name, VNODE_NAME_MAX) >= VNODE_NAME_MAX)
	{
		return -ERROR_INVALID;
	}

	auto ret = this->find_first([](vnode_base* vn, const void* key)
	{
	  return strncmp(vn->name_buf, (char*)key, VNODE_NAME_MAX) == 0;
	}, name);

	if (ret == nullptr)
	{
		return -ERROR_NO_ENTRY;
	}
	else
	{
		return ret;
	}
}
