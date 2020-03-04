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

void lock::spinlock_acquire(spinlock *lock)
{
    pushcli();
    if (spinlock_holding(lock))
    {
        kdebug::kdebug_dump_lock_panic(lock);
    }

    while (xchg(&lock->locked, 1u) != 0)
        ;

    lock->cpu = cpu;
    kdebug::kdebug_getcallerpcs(16, lock->pcs);
}

void lock::spinlock_release(spinlock *lock)
{
    if (!spinlock_holding(lock))
    {
        KDEBUG_RICHPANIC("Release a not-held spinlock.\n",
                             "KERNEL PANIC",
                             false,
                             "Lock's name: %s", lock->name);
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
        KDEBUG_RICHPANIC("Can't be called if interrupts are enabled",
                             "KERNEL PANIC: SPINLOCK",
                             false,
                             "");
    }

    --cpu->nest_pushcli_depth;
    KDEBUG_ASSERT(cpu->nest_pushcli_depth >= 0);


    if (cpu->nest_pushcli_depth == 0 && cpu->intr_enable)
    {
        sti();
    }
}
