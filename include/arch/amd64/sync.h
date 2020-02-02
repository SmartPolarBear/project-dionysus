#if !defined(__INCLUDE_ARCH_AMD64_SYNC)
#define __INCLUDE_ARCH_AMD64_SYNC

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

#define local_intrrupt_save(x) \
    do                     \
    {                      \
        x = __intr_save(); \
    } while (0)

#define local_intrrupt_restore(x) __intr_restore(x);

#endif // __INCLUDE_ARCH_AMD64_SYNC
