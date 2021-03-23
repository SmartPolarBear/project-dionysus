#include "arch/amd64/cpu/port_io.h"

#include "system/types.h"
#include "system/memlayout.h"

#include "fs/fs.hpp"
#include "fs/vfs/vfs.hpp"
#include "fs/ext2/ext2.hpp"

#include "drivers/pci/pci.hpp"
#include "drivers/pci/pci_device.hpp"
#include "drivers/pci/pci_header.hpp"
#include "drivers/pci/pci_capability.hpp"
#include "drivers/ahci/ahci.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "../libs/basic_io/include/builtin_text_io.hpp"

using namespace kbl;
using namespace file_system;

list_head fs_class_head;
list_head fs_mount_head;

vfs_io_context the_kernel_io_context{ nullptr, 0, 0 };
vfs_io_context* const file_system::kernel_io_context = &the_kernel_io_context;

// workaround the problem that it's impossible to use current
// implementation of intrusive linked list with non-pod types

struct fs_class_node
{
	file_system::fs_class_base* fs_class;

	list_head link;
};

char testbuf[1024];
static inline error_code test_read()
{
	file_object ob;

	auto err = kernel_io_context->open_at(&ob, nullptr, "hello", IOCTX_FLG_RDONLY, S_IFREG);
	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	auto read_ret = kernel_io_context->read(&ob, testbuf, 1024);
	if (has_error(read_ret))
	{
		return get_error_code(read_ret);
	}

	auto size = get_result(read_ret);

	kdebug::kdebug_log("Read size=%lld, first 128 byte:\n", size);
	for (int i = 0; i < 128; i++)
	{
		kdebug::kdebug_log("%x", testbuf[i]);
		if ((i + 1) % 24 == 0)kdebug::kdebug_log("\n");
		else kdebug::kdebug_log(" ");
	}
	kdebug::kdebug_log("\n");

	return ERROR_SUCCESS;
}

static inline error_code test_rw()
{
	file_object ob;

	auto err = kernel_io_context->open_at(&ob, nullptr, "test.txt", IOCTX_FLG_RDWR | IOCTX_FLG_CREATE, S_IFREG);
	if (err != ERROR_SUCCESS)
	{
		return err;
	}

	memset(testbuf, 0, sizeof(testbuf));
	strcpy(testbuf, "Miao miao miao miao miao!!!");

	auto write_ret = kernel_io_context->write(&ob, testbuf, strlen(testbuf));
	if (has_error(write_ret))
	{
		return get_error_code(write_ret);
	}

	//TODO: implement seek
	ob.pos = 0;

	memset(testbuf, 0, sizeof(testbuf));

	auto read_ret = kernel_io_context->read(&ob, testbuf, 25);
	if (has_error(read_ret))
	{
		return get_error_code(read_ret);
	}

	auto size = get_result(read_ret);
	kdebug::kdebug_log("Read size=%lld, first %lld byte:\n", size, size);
	for (size_t i = 0; i < size; i++)
	{
		kdebug::kdebug_log("%x(%c)", testbuf[i], testbuf[i]);
		if ((i + 1) % 24 == 0)kdebug::kdebug_log("\n");
		else kdebug::kdebug_log(" ");
	}
	kdebug::kdebug_log("\n");

	return ERROR_SUCCESS;
}

static inline error_code kernel_io_context_init()
{
	auto dev_find_ret = device_find_first(DC_BLOCK, "sda1");
	if (has_error(dev_find_ret))
	{
		return get_error_code(dev_find_ret);
	}

	auto vnode = get_result(dev_find_ret);

	auto err = kernel_io_context->mount("/", vnode->get_dev(), FSC_EXT2, 0, nullptr);

	if (err != ERROR_SUCCESS)
	{
		return err;
	}

//	err = test_read();
//	if (err != ERROR_SUCCESS)
//	{
//		return err;
//	}
//
//	err = test_rw();
//	if (err != ERROR_SUCCESS)
//	{
//		return err;
//	}

	return ERROR_SUCCESS;
}

PANIC void file_system::fs_init()
{


	// Initialize list heads
	list_init(&fs_class_head);
	list_init(&fs_mount_head);

	// register file system classes
	g_ext2fs.register_this();

	// Initialize devfs root to load real hardware
	auto ret = file_system::init_devfs_root();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	// Initialize the fs hardware
	ret = ahci::ahci_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	ret = file_system::vfs_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

	ret = kernel_io_context_init();
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, false, "");
	}

}

error_code_with_result<fs_instance*> file_system::fs_create(fs_class_base* fs_class,
	device_class* dev,
	size_t flags,
	const char* data)
{
	fs_instance* fs_ins = new(std::nothrow) fs_instance;
	if (fs_ins == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	fs_ins->fs_class = fs_class;
	fs_ins->dev = dev;
	fs_ins->flags = flags;

	error_code ret = fs_class->initialize(fs_ins, data);
	if (ret == ERROR_UNSUPPORTED)
	{
		kdebug::kdebug_warning("%s do not have a initialize function!", fs_class->get_name());
	}
	else if (ret != ERROR_SUCCESS && ret != ERROR_UNSUPPORTED)
	{
		delete fs_ins;
		return ret;
	}

	list_add(&fs_ins->link, &fs_mount_head);

	return fs_ins;
}

error_code file_system::fs_register(fs_class_base* fscls)
{
	if (fs_find(fscls->get_id()) != nullptr)
	{
		return -ERROR_REWRITE;
	}

	fs_class_node* node = new(std::nothrow) fs_class_node
		{
			.fs_class=fscls,
		};

	list_add(&node->link, &fs_class_head);
	return ERROR_SUCCESS;
}

file_system::fs_class_base* file_system::fs_find(fs_class_id id)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		auto entry = list_entry(iter, fs_class_node, link);
		if (entry->fs_class->get_id() == id)
		{
			return entry->fs_class;
		}
	}
	return nullptr;
}

file_system::fs_class_base* file_system::fs_find(const char* name)
{
	list_head* iter = nullptr;
	list_for(iter, &fs_class_head)
	{
		auto entry = list_entry(iter, fs_class_node, link);
		if (strcmp(entry->fs_class->get_name(), name) == 0)
		{
			return entry->fs_class;
		}
	}
	return nullptr;
}

