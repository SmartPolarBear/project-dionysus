#if !defined(__INCLUDE_DRIVERS_KDEBUG_H)
#define __INCLUDE_DRIVERS_KDEBUG_H

#include "sys/types.h"

namespace kdebug
{
void kdebug_panic(const char *fmt, ...);
void kdebug_getcallerpcs(size_t buflen, uintptr_t pcs[]);

// panic with line number and file name
// to make __FILE__ and __LINE__ macros works right, this must be a macro as well.
#define KDEBUG_GENERALPANIC(str) \
    kdebug::kdebug_panic("KDEBUG_GENERALPANIC:\nIn file: %s, line: %d\n%s", __FILE__, __LINE__, str)

// panic for not implemented functions
#define KDEBUG_NOT_IMPLEMENTED \
    kdebug::kdebug_panic("KDEBUG_NOT_IMPLEMENTED:\nIn file: %s, line: %d\nThe function \"%s\" is not implemented.", __FILE__, __LINE__, __FUNCTION__)

// panic the kernel if the condition isn't equal to 1
#define KDEBUG_ASSERT(cond)                                                                                                \
    do                                                                                                                     \
    {                                                                                                                      \
        if (!(cond))                                                                                                       \
            kdebug::kdebug_panic("ASSERT_PANIC:\nIn file: %s, line: %d\n\"%s\" is required. ", __FILE__, __LINE__, #cond); \
    } while (0)

} // namespace kdebug

#endif // __INCLUDE_DRIVERS_KDEBUG_H
