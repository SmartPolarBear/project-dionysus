#pragma once

//#include "drivers/acpi/cpu.h"
struct cpu_struct;
#include "system/types.h"

namespace lock
{
struct spinlock
{
    uint32_t locked;

    const char *name;
    cpu_struct *cpu;
    uintptr_t pcs[16];
};

void spinlock_acquire(spinlock *lock);
bool spinlock_holding(spinlock *lock);
void spinlock_initlock(spinlock *lock, const char *name);
void spinlock_release(spinlock *lock);


} // namespace lock
