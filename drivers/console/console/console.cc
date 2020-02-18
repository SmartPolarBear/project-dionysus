#include "../console.h"
#include "../cga/cga.h"

#include "drivers/lock/spinlock.h"


using libk::list_add;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

// spinlock
using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_holding;
using lock::spinlock_initlock;
using lock::spinlock_release;

struct
{
    list_head devs_head;
    spinlock cons_lock;
    bool lock_enable;
} cons;

void console_init()
{
    // enable the linked-list of all console devices
    list_init(&cons.devs_head);

    // initialize the lock
    spinlock_initlock(&cons.cons_lock, "console_main");
    cons.lock_enable = false;

    // add the built-in device drivers
    list_add(&cga_dev.dev_link,&cons.devs_head);
}