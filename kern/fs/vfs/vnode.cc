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

struct vnode_child_node
{
	file_system::vnode_base* vnode;
	list_head list;
};

using namespace memory::kmem;
using namespace libkernel;

memory::kmem::kmem_cache* vnode_child_node_cache = nullptr;



error_code file_system::vnode_init()
{
	vnode_child_node_cache = kmem_cache_create("vnode_child", sizeof(vnode_child_node));

	if (vnode_child_node_cache == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	return ERROR_SUCCESS;
}

error_code file_system::vnode_base::attach(file_system::vnode_base* child)
{
	vnode_child_node* child_node = reinterpret_cast<vnode_child_node*>(kmem_cache_alloc(vnode_child_node_cache));
	child_node->vnode = child;
	list_add(&child_node->list, &this->child_head);
	return 0;
}

error_code file_system::vnode_base::detach(file_system::vnode_base* node)
{
	list_head* iter = nullptr, * t = nullptr;
	list_for_safe(iter, t, &this->child_head)
	{
		auto n = list_entry(iter, vnode_child_node, list);
		if (n->vnode == node)
		{
			list_remove(iter);
			break;
		}
	}
	return 0;
}

error_code_with_result<file_system::vnode_base*> file_system::vnode_base::lookup_child(const char* name)
{
	list_head* iter = nullptr;
	list_for(iter, &this->child_head)
	{
		auto n = list_entry(iter, vnode_child_node, list);
		if (strcmp(n->vnode->get_name(),name)==0)
		{
			return n->vnode;
		}
	}

	return -ERROR_NO_ENTRY;
}
