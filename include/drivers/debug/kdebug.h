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
#define KDEBUG_GENERALPANIC(str) \
    kdebug::kdebug_panic("KDEBUG_GENERALPANIC:\nIn file: %s, line: %d\n%s", __FILE__, __LINE__, str)

#define KDEBUG_GENERALPANIC_WITH_RETURN_ADDR(str) \
    kdebug::kdebug_panic("KDEBUG_GENERALPANIC:\nIn file: %s, line: %d\nreturn address: 0x%x\n%s", __FILE__, __LINE__, __builtin_return_address(0), str)

// panic for not implemented functions
#define KDEBUG_NOT_IMPLEMENTED \
    kdebug::kdebug_panic("KDEBUG_NOT_IMPLEMENTED:\nIn file: %s, line: %d\nThe function \"%s\" is not implemented.", __FILE__, __LINE__, __FUNCTION__)

// pnaic that not move the cursor
#define KDEBUG_FOLLOWPANIC(str) \
    kdebug::kdebug_panic2("KDEBUG_GENERALPANIC:\nIn file: %s, line: %d\n%s", false, __FILE__, __LINE__, str)

// panic the kernel if the condition isn't equal to 1
#define KDEBUG_ASSERT(cond)                                                                                                \
    do                                                                                                                     \
    {                                                                                                                      \
        if (!(cond))                                                                                                       \
            kdebug::kdebug_panic("ASSERT_PANIC:\nIn file: %s, line: %d\n\"%s\" is required. ", __FILE__, __LINE__, #cond); \
    } while (0)

} // namespace kdebug

#include "drivers/debug/hresult.h"

#endif // __INCLUDE_DRIVERS_KDEBUG_H
