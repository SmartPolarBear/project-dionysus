#if !defined(__INCLUDE_DRIVERS_SPINLOCK_H)
#define __INCLUDE_DRIVERS_SPINLOCK_H

#include "drivers/acpi/cpu.h"
#include "sys/types.h"

namespace lock
{
struct spinlock
{
    uint32_t locked;

    const char *name;
    cpu_info *cpu;
    uintptr_t pcs[16];
};

void spinlock_acquire(spinlock *lock);
bool spinlock_holding(spinlock *lock);
void spinlock_initlock(spinlock *lock, const char *name);
void spinlock_release(spinlock *lock);
void pushcli(void);
void popcli(void);

class mutex
{
private:
    using lock_val_type = volatile uint32_t;
    lock_val_type lockval;
public:
    void lock(void);
    void unlock(void);
}

} // namespace lock

#endif // __INCLUDE_DRIVERS_SPINLOCK_H
