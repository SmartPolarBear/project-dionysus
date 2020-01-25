#if !defined(__INCLUDE_DRIVERS_KDEBUG_H)
#define __INCLUDE_DRIVERS_KDEBUG_H

#include "sys/types.h"

#include "drivers/lock/spinlock.h"

namespace kdebug
{
extern bool panicked;

void kdebug_panic(const char *fmt, ...);
// use uint32_t for the bool value to make va_args happy.
void kdebug_panic2(const char *fmt, uint32_t topleft, ...);
void kdebug_getcallerpcs(size_t buflen, uintptr_t pcs[]);

void kdebug_dump_lock_panic(lock::spinlock *lock);

// panic with line number and file name
// to make __FILE__ and __LINE__ macros works right, this must be a macro as well.
#define KDEBUG_RICHPANIC(msg, title, topleft, add_fmt, args...) \
    kdebug::kdebug_panic2(title ":\nIn file: %s, line: %d\nIn scope: %s\nMessage:\n%s\n" add_fmt, topleft, __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, ##args)

#define KDEBUG_GENERALPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", true, "")

// pnaic that not move the cursor
#define KDEBUG_FOLLOWPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", false, "")

// panic for not implemented functions
#define KDEBUG_NOT_IMPLEMENTED \
    KDEBUG_RICHPANIC("The function is not implemented", "KDEBUG_NOT_IMPLEMENTED", true, "");

// panic the kernel if the condition isn't equal to 1
#define KDEBUG_ASSERT(cond)                                                 \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            KDEBUG_RICHPANIC("Assertion failed",                            \
                             "ASSERT_PANIC",                                \
                             true,                                          \
                             "The expression \" %s \" is expected to be 1", \
                             #cond);                                        \
        }                                                                   \
    } while (0)

} // namespace kdebug

#include "drivers/debug/hresult.h"

#endif // __INCLUDE_DRIVERS_KDEBUG_H
