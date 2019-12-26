#if !defined(__INCLUDE_DRIVERS_SPINLOCK_H)
#define __INCLUDE_DRIVERS_SPINLOCK_H

#include "drivers/acpi/cpu.h"
#include "sys/types.h"

namespace lock
{
struct spinlock
{
    uint32_t locked;

    char *name;
    cpu_info *cpu;
    uintptr_t pcs[16];
};

void spinlock_acquire(spinlock *);
bool spinlock_holding(spinlock *);
void spinlock_initlock(spinlock *, char *);
void spinlock_release(spinlock *);
void pushcli(void);
void popcli(void);

} // namespace lock

#endif // __INCLUDE_DRIVERS_SPINLOCK_H
