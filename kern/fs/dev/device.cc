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
#include "fs/ext2/ext2.hpp"

#include "libkernel/console/builtin_text_io.hpp"

#include <cstring>
#include <cmath>
#include <algorithm>
#include <fs/vfs/vfs.hpp>

using namespace file_system;
using namespace memory::kmem;

[[maybe_unused]] char block_dev_sd_idx_char = 'a';
[[maybe_unused]] char block_dev_cd_idx_char = 'a';
[[maybe_unused]] char block_dev_hd_idx_char = 'a';

[[maybe_unused]]const char* block_dev_sd_name = "sd ";
[[maybe_unused]]const char* block_dev_cd_name = "cd ";
[[maybe_unused]]const char* block_dev_hd_name = "hd ";

struct vnode_base_node
{
	vnode_base* vnode;
	list_head link;
};

kmem_cache* vnode_base_node_cache = nullptr;

vnode_base* devfs_root = nullptr;

static inline error_code device_generate_name(device_class_id cls, size_t sbcls, OUT char* namebuf)
{
	if (namebuf == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (cls == DC_BLOCK)
	{
		switch (sbcls)
		{
		case DBT_CDx:
			if (block_dev_cd_idx_char > 'z')
			{
				return -ERROR_UNSUPPORTED;
			}
			strncpy(namebuf, block_dev_cd_name, 3);
			namebuf[3] = '\0';// ensure null-terminated

			namebuf[2] = block_dev_cd_idx_char++;

			break;

		case DBT_SDx:
			if (block_dev_sd_idx_char > 'z')
			{
				return -ERROR_UNSUPPORTED;
			}
			strncpy(namebuf, block_dev_sd_name, 3);
			namebuf[3] = '\0';// ensure null-terminated

			namebuf[2] = block_dev_sd_idx_char++;
			break;

		default:
			return -ERROR_INVALID;
		}

		return ERROR_SUCCESS;
	}
	return -ERROR_INVALID;
}

error_code file_system::device_add(device_class_id cls, size_t subcls, device_class& dev, const char* name)
{
//	ext2_instance.dev = &dev;

	char* node_name = new char[64];
	error_code ret = ERROR_SUCCESS;

	if (!name)
	{
		if ((ret = device_generate_name(cls, subcls, node_name)) != ERROR_SUCCESS)
		{
			delete[] node_name;
			return ret;
		}

		name = node_name;
	}

	vnode_base* node = nullptr;
	if (cls == DC_BLOCK)
	{
		node = new dev_fs_node(vnode_types::VNT_BLK, name);
	}
	else if (cls == DC_CHAR)
	{
		node = new dev_fs_node(vnode_types::VNT_CHR, name);
	}
	else
	{
		delete[] node_name;
		return -ERROR_INVALID;
	}

	// we have copied its content, so release the memory
	delete[] node_name;

	node->dev = &dev;

	// Use inode number to store full device class:subclass
	node->inode_id = ((uint32_t)cls) | ((uint64_t)subcls << 32u);

	node->flags |= VNF_MEMORY;

	// Default permissions for devices
	node->mode = DEVICE_DEFAULT_MODE;
	node->uid = 0;
	node->gid = 0;

//	auto wrapper = reinterpret_cast<vnode_base_node*>(kmem_cache_alloc(vnode_base_node_cache));
//
//	wrapper->vnode = node;
//	kernel::list_add(&wrapper->link, &devfs_root->child_head);
	devfs_root->attach(node);

	if (dev.features & DFE_HAS_PARTITIONS)
	{
		dev.enumerate_partitions(*node);
	}

	return ret;
}

error_code_with_result<vnode_base*> file_system::device_find_first(device_class_id cls, const char* name)
{
	if (!devfs_root)
	{
		return -ERROR_DEV_NOT_FOUND;
	}

	auto child_ret = devfs_root->lookup_child(name);
	if (has_error(child_ret))
	{
		return get_error_code(child_ret);
	}

	auto vnode = get_result(child_ret);
	if (vnode->type == vnode_types::VNT_LNK)
	{
		//TODO: multiple link
		vnode = vnode->get_link_target();
	}

	if (cls == DC_BLOCK && vnode->type != vnode_types::VNT_BLK)
	{
		return -ERROR_DEV_NOT_FOUND;
	}

	if (cls == DC_CHAR && vnode->type != vnode_types::VNT_CHR)
	{
		return -ERROR_DEV_NOT_FOUND;
	}

	return vnode;
}

error_code file_system::device_add_link(const char* name, vnode_base* to)
{
	dev_fs_node* node = new dev_fs_node(vnode_types::VNT_LNK, name);
	node->link_target.node_target = to;
	node->flags |= VNF_MEMORY;

	node->mode = 0777;
	node->uid = 0;
	node->gid = 0;

	devfs_root->attach(node);

	return 0;
}

error_code file_system::device_add_link(const char* name, vnode_link_getter_type getter)
{
	dev_fs_node* node = new dev_fs_node(vnode_types::VNT_LNK, name);
	node->link_target.link_getter = getter;
	node->flags |= VNF_MEMORY | VNF_PER_PROCESS;

	node->mode = 0777;
	node->uid = 0;
	node->gid = 0;

	devfs_root->attach(node);

	return 0;
}

error_code file_system::init_devfs_root()
{
	devfs_root = static_cast<vnode_base*>(new dev_fs_node(vnode_types::VNT_BLK, nullptr));

	if (!devfs_root)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	devfs_root->flags |= VNF_MEMORY;
	devfs_root->mode = ROOT_DEFAULT_MODE;

	devfs_root->uid = 0;
	devfs_root->gid = 0;

	vnode_base_node_cache = kmem_cache_create("vnode_node_base", sizeof(vnode_base_node));

	return ERROR_SUCCESS;
}


