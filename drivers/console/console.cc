#include "drivers/console/console.h"
#include "drivers/console/cga.h"
#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"
#include "sys/types.h"

#include <stdarg.h>

using putc_function_type = void (*)(uint32_t);

putc_function_type putc_funcs[] = {console::cga_putc};

static inline void putc(uint32_t c)
{
    for (auto func : putc_funcs)
    {
        func(c);
    }
}

void console::puts(const char *str)
{
    while (*str != '\0')
    {
        putc(*str);
        ++str;
    }
}

char nbuf[64] = {};
void console::printf(const char *fmt, ...)
{
    va_list ap;
    int i, c; //, locking;
    const char *s;

    va_start(ap, fmt);

    //TODO: acquire the lock
    if (fmt == 0)
    {
        //TODO : panic the kernel
        return;
    }


    for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
    {
        if (c != '%')
        {
            putc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c)
        {
        case 'd':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 10);
            for (size_t i = 0; nbuf[i]; i++)
            {
                putc(nbuf[i]);
            }
            break;
        case 'x':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                putc(nbuf[i]);
            }
            break;
        case 'p':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, size_t), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                putc(nbuf[i]);
            }
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                putc(*s);
            break;
        case '%':
            putc('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            putc('%');
            putc(c);
            break;
        }
    }

    //TODO : release the lock
}

void console::console_init(void)
{
}