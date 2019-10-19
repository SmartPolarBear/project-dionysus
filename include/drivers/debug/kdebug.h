#if !defined(__INCLUDE_DRIVERS_KDEBUG_H)
#define __INCLUDE_DRIVERS_KDEBUG_H

namespace kdebug
{
void kdebug_panic(const char *fmt, ...);

// panic with line number and file name
// to make __FILE__ and __LINE__ macros works right, this must be a macro as well.
#define KDEBUG_GENERALPANIC(str) \
    kdebug::kdebug_panic("PANIC: %s %d %s %d %s", "file:", __FILE__, "line:", __LINE__, str)

} // namespace kdebug

#endif // __INCLUDE_DRIVERS_KDEBUG_H
