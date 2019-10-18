#if !defined(__INCLUDE_DRIVERS_CONSOLE_UART_H)
#define __INCLUDE_DRIVERS_CONSOLE_UART_H
#include "sys/types.h"
namespace console
{
void uart_putc(uint32_t c);
} // namespace console

#endif // __INCLUDE_DRIVERS_CONSOLE_UART_H
