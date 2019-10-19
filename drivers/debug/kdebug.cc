#include "drivers/debug/kdebug.h"
#include "drivers/console/console.h"
#include "sys/types.h"

#include <stdarg.h>

static bool panicked = false;

static void print_stackframe()
{
}

void kdebug::kdebug_panic(const char *fmt, ...)
{
    // preparation : change cga color to draw attention

    

    // first, print the given imformation.
    va_list ap;
    int i, c; //, locking;
    const char *s;

    va_start(ap, fmt);

    if (fmt)
    {

        for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
        {
            if (c != '%')
            {
                console::printf("%c", c);
                continue;
            }
            c = fmt[++i] & 0xff;
            if (c == 0)
                break;
            switch (c)
            {
            case 'c':
            case 'd':
                console::printf("%d", va_arg(ap, int));
                break;
            case 'x':
                console::printf("%x", va_arg(ap, int));
                break;
            case 'p':
                console::printf("%p", va_arg(ap, size_t));
                break;
            case 's':
                console::printf("%s", va_arg(ap, char *));
                break;
            case '%':
                console::printf("%");
                break;
            default:
                console::printf("%%%c", c);
                break;
            }
        }
    }

    print_stackframe();

    va_end(ap);
}