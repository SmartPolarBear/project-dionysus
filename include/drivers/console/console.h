#if !defined(__INCLUDE_DRIVERS_CONSOLE_H)
#define __INCLUDE_DRIVERS_CONSOLE_H
namespace console
{
void printf(const char *fmt, ...);
void puts(const char *str);
void console_init(void);
} // namespace console

#endif // __INCLUDE_DRIVERS_CONSOLE_H
