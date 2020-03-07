#pragma once

#include "arch/amd64/regs.h"
#include "arch/amd64/x86.h"

static inline bool
__intr_save(void)
{
    if (read_eflags() & EFLAG_IF)
    {
        cli();
        return 1;
    }
    return 0;
}

static inline void
__intr_restore(bool flag)
{
    if (flag)
    {
        sti();
    }
}

#define software_pushcli(x) \
    do                      \
    {                       \
        x = __intr_save();  \
    } while (0)

#define software_popcli(x) \
    do                     \
    {                      \
        __intr_restore(x); \
    } while (0)
