#include "../cga/cga.h"

#include "arch/amd64/cpu/cpu.h"

#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "kbl/data/pod_list.h"

using namespace kbl;

using namespace lock;
using namespace console;

struct kconsole
{
	list_head devs_head;
	spinlock_struct cons_lock;
	bool lock_enable;

	console_colors background;
	console_colors foreground;
} cons;

void console::console_add_dev(IN console_dev* dev)
{
	list_add(&dev->dev_link, &cons.devs_head);
}

void console::console_remove_internal_devs()
{
	list_remove(&cga_dev.dev_link);
}

void console::console_init()
{
	// enable the linked-list of all console devices
	list_init(&cons.devs_head);

	// initialize the lock
	spinlock_initialize_lock(&cons.cons_lock, "console_main");
	cons.lock_enable = true;

	// add the built-in device drivers
	list_add(&cga_dev.dev_link, &cons.devs_head);

	cons.background = CONSOLE_COLOR_BLACK;
	cons.foreground = CONSOLE_COLOR_LIGHT_GREY;
}

void console::console_set_color(console_colors background, console_colors foreground)
{
	cons.background = background;
	cons.foreground = foreground;
}

void console::console_write_char(char c)
{
	// acquire the lock
	bool locking = cons.lock_enable;
	if (locking)
	{
		spinlock_acquire(&cons.cons_lock);
	}

	list_head* iter = nullptr;
	list_for(iter, &cons.devs_head)
	{
		auto consdev = container_of(iter, console_dev, dev_link);

		consdev->write_char(c, cons.background, cons.foreground);
	}

	// release the lock
	if (locking)
	{
		spinlock_release(&cons.cons_lock);
	}
}

void console::cosnole_write_string(const char* str, size_t len)
{
	// acquire the lock
	bool locking = cons.lock_enable;
	if (locking)
	{
		spinlock_acquire(&cons.cons_lock);
	}

	for (size_t i = 0; i < len; i++)
	{
		list_head* iter = nullptr;
		list_for(iter, &cons.devs_head)
		{
			auto cons_dev = container_of(iter, console_dev, dev_link);

			cons_dev->write_char(*(str + i), cons.background, cons.foreground);
		}
	}

	// release the lock
	if (locking)
	{
		spinlock_release(&cons.cons_lock);
	}
}

void console::cosnole_write_string(const char* str)
{
	// acquire the lock
	bool locking = cons.lock_enable;
	if (locking)
	{
		spinlock_acquire(&cons.cons_lock);
	}

	while (*str != '\0')
	{
		list_head* iter = nullptr;
		list_for(iter, &cons.devs_head)
		{
			auto consdev = container_of(iter, console_dev, dev_link);

			consdev->write_char(*str, cons.background, cons.foreground);
		}
		++str;
	}

	// release the lock
	if (locking)
	{
		spinlock_release(&cons.cons_lock);
	}
}

void console::console_set_pos(cursor_pos pos)
{
	// acquire the lock
	bool locking = cons.lock_enable;
	if (locking)
	{
		spinlock_acquire(&cons.cons_lock);
	}

	list_head* iter = nullptr;
	list_for(iter, &cons.devs_head)
	{
		auto consdev = container_of(iter, console_dev, dev_link);

		consdev->set_cursor_pos(pos);
	}

	if (locking)
	{
		spinlock_release(&cons.cons_lock);
	}
}

void console::console_panic_lock()
{
	if (lock::arch_spinlock_try_lock(&cons.cons_lock.arch))
	{
		// infinite loop to halt the cpu
		for (;;)
			hlt();
	}
}

void console::console_set_lock(bool enable)
{
	cons.lock_enable = enable;
	if (!enable)
	{
		if (spinlock_holding(&cons.cons_lock))
		{
			spinlock_release(&cons.cons_lock);
		}
	}
}

bool console::console_get_lock()
{
	return cons.lock_enable;
}
