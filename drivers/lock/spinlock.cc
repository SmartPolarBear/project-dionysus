#include "arch/amd64/x86.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"


using lock::spinlock;

void lock::spinlock_initlock(spinlock *splk, const char *name)
{
    splk->name = name;
    splk->locked = 0u;
    splk->cpu = nullptr;
}

static inline void dump_callstack_and_panic(const spinlock *lock)
{
    // disable the lock of console
    console::console_debugdisablelock();
    console::console_setpos(0);

    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);

    console::printf("lock %s has been held.\nCall stack:\n", lock->name);

    for (auto cs : lock->pcs)
    {
        console::printf("%p ", cs);
    }

    console::printf("\n");

    KDEBUG_FOLLOWPANIC("acquire");
}

void lock::spinlock_acquire(spinlock *lock)
{
    pushcli();
    if (spinlock_holding(lock))
    {
        dump_callstack_and_panic(lock);
    }
    __sync_synchronize();

    while (xchg(&lock->locked, 1u) != 0)
        ;

    lock->cpu = cpu;
    kdebug::kdebug_getcallerpcs(16, lock->pcs);
}

void lock::spinlock_release(spinlock *lock)
{
    if (!spinlock_holding(lock))
    {
        KDEBUG_GENERALPANIC("Release a not-held spinlock.\n");
    }

    lock->pcs[0] = 0;
    lock->cpu = nullptr;

    xchg(&lock->locked, 0u);

    popcli();
}

bool lock::spinlock_holding(spinlock *lock)
{
    return lock->locked && lock->cpu == cpu;
}

void lock::pushcli(void)
{
    auto eflags = read_eflags();
    cli();
    if (cpu->nest_pushcli_depth++ == 0)
    {
        cpu->intr_enable = eflags & EFLAG_IF;
    }
}

void lock::popcli(void)
{
    if (read_eflags() & EFLAG_IF)
    {
        KDEBUG_GENERALPANIC("popcli - interruptible");
    }

    if (--cpu->nest_pushcli_depth < 0)
    {
        KDEBUG_GENERALPANIC("popcli");
    }

    if (cpu->nest_pushcli_depth == 0 && cpu->intr_enable)
    {
        sti();
    }
}
