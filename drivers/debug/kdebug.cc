#include "drivers/debug/kdebug.h"
#include "arch/amd64/x86.h"
#include "drivers/console/console.h"
#include "sys/memlayout.h"
#include "sys/types.h"

#include <stdarg.h>

static bool panicked = false;

void kdebug::kdebug_getcallerpcs(void *addr, size_t buflen, uintptr_t pcs[])
{
    uintptr_t *ebp = (uintptr_t *)addr - 2;
    size_t i = 0;
    for (; i < buflen; i++)
    {
        if (ebp == 0 || ebp < (uintptr_t *)KERNEL_VIRTUALBASE || ebp == (uintptr_t *)VIRTUALADDR_LIMIT)
        {
            break;
        }
        pcs[i] = ebp[1];           // saved %eip
        ebp = (uintptr_t *)ebp[0]; // saved %ebp
    }

    for (; i < buflen; i++)
    {
        pcs[i] = 0;
    }
}

void kdebug::kdebug_panic(const char *fmt, ...)
{
    // disable interrupts
    cli();

    //TODO: disable the lock of console

    // change cga color and reset cursor to draw attention
    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);
    console::console_setpos(0);
    

    // first, print the given imformation.
    va_list ap;
    int c;
    const char *s;

    va_start(ap, fmt);

    if (fmt)
    {

        for (size_t i = 0; (c = fmt[i] & 0xff) != 0; i++)
        {
            if (c != '%')
            {
                console::putc(c);
                continue;
            }
            c = fmt[++i] & 0xff;
            if (c == 0)
                break;
            switch (c)
            {
            case 'c':
                console::printf("%c", va_arg(ap, char));
                break;
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
                console::putc('%');
                console::putc(c);

                break;
            }
        }
    }

    va_end(ap);

    uintptr_t pcs[32] = {0};
    kdebug_getcallerpcs(&fmt, 32, pcs);

    console::printf("\n");
    for (auto pc : pcs)
    {
        console::printf("%p ", pc);
    }

    // set global panic state for other cpu
    panicked = true;
    // infinite loop
    for (;;)
        ;
}