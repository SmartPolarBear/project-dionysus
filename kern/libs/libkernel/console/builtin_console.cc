#include "libraries/libkernel/console/builtin_console.hpp"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "libraries/libc/stdlib.h"

#include <cstring>

#include "system/types.h"

// spinlock
using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_holding;
using lock::spinlock_initlock;
using lock::spinlock_release;

spinlock printf_lock;

__attribute__((constructor)) void printf_init(void)
{
    spinlock_initlock(&printf_lock, "printf");
}

void PutChar(const char *str)
{
    auto len = strlen(str);
    console::cosnole_write_string(str, len);
}

void PutChar(char c)
{
    console::console_write_char(c);
}

void WriteFormat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    VaListWriteFormat(fmt, ap);

    va_end(ap);
}

// buffer for converting ints with itoa
constexpr size_t MAXNUMBER_LEN = 256;
char nbuf[MAXNUMBER_LEN] = {};

void VaListWriteFormat(const char *fmt, va_list ap)
{
    bool locking = console::console_get_lock();
    if (locking)
    {
        spinlock_acquire(&printf_lock);
    }

    if (fmt == 0)
    {
        KDEBUG_RICHPANIC("Invalid null format strings",
                         "KERNEL PANIC: BUILTIN CONSOLE",
                         false,
                         "");
    }

    auto char_data = [](char d) -> decltype(d & 0xFF) {
        return d & 0xFF;
    };

    size_t i = 0, c = 0;
    auto next_char = [&i, fmt, char_data](void) -> char {
        return char_data(fmt[++i]);
    };

    char ch = 0;
    const char *s = nullptr;

    for (i = 0; (c = char_data(fmt[i])) != 0; i++)
    {

        if (c != '%')
        {
            console::console_write_char(c);
            continue;
        }

        c = next_char();

        if (c == 0)
        {
            break;
        }

        // reset buffer for itoa()
        memset(nbuf, 0, sizeof(nbuf));

        switch (c)
        {
        case 'c':
        { // this va_arg uses int
            // otherwise, a warning will be given, saying
            // warning: second argument to 'va_arg' is of promotable type 'char'; this va_arg has undefined behavior because arguments will be promoted to 'int
            ch = va_arg(ap, int);
            console::console_write_char(char_data(ch));
            break;
        }
        case 'f':
        {
            //FIXME: va_arg(ap, double) always return wrong value
            KDEBUG_RICHPANIC("%f flag is disabled because va_arg(ap, double) always return wrong value",
                             "KERNEL PANIC: BUILTIN CONSOLE",
                             false,
                             "");

            size_t len = ftoa_ex(va_arg(ap, double), nbuf, 10);
            for (size_t i = 0; i < len; i++)
            {
                console::console_write_char(nbuf[i]);
            }
            break;
        }
        case 'd':
        {
            size_t len = itoa_ex(nbuf, va_arg(ap, int), 10);
            for (size_t i = 0; i < len; i++)
            {
                console::console_write_char(nbuf[i]);
            }
            break;
        }
        case 'l':
        {
            char nextchars[2] = {0};
            nextchars[0] = next_char();
            nextchars[1] = next_char();

            if (nextchars[0] == 'l' && nextchars[1] == 'd')
            {
                size_t len = itoa_ex(nbuf, va_arg(ap, unsigned long long), 10);
                for (size_t i = 0; i < len; i++)
                {
                    console::console_write_char(nbuf[i]);
                }
            }
            else
            {
                // Print unknown % sequence to draw attention.
                console::console_write_char('%');
                console::console_write_char('l');
                console::console_write_char(nextchars[0]);
                console::console_write_char(nextchars[1]);
            }
            break;
        }
        case 'x':
        {
            size_t len = itoa_ex(nbuf, va_arg(ap, int), 16);
            for (size_t i = 0; i < len; i++)
            {
                console::console_write_char(nbuf[i]);
            }
            break;
        }
        case 'p':
        {
            static_assert(sizeof(size_t *) == sizeof(size_t));

            size_t len = itoa_ex(nbuf, va_arg(ap, size_t), 16);
            for (size_t i = 0; i < len; i++)
            {
                console::console_write_char(nbuf[i]);
            }
            break;
        }
        case 's':
        {
            if ((s = va_arg(ap, char *)) == 0)
            {
                s = "(null)";
            }
            for (; *s; s++)
            {
                console::console_write_char(*s);
            }
            break;
        }
        case '%':
        {
            console::console_write_char('%');
            break;
        }
        default:
        {
            // Print unknown % sequence to draw attention.
            console::console_write_char('%');
            console::console_write_char(c);
            break;
        }
        }
    }

    if (locking)
    {
        spinlock_release(&printf_lock);
    }
}
