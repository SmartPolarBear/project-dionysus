#include "drivers/debug/kdebug.h"
#include "arch/amd64/x86.h"
#include "drivers/console/console.h"
#include "sys/memlayout.h"
#include "sys/types.h"

#include <stdarg.h>

bool kdebug::panicked = false;

// read information from %rbp and retrive pcs
void kdebug::kdebug_getcallerpcs(size_t buflen, uintptr_t pcs[])
{
    uintptr_t *ebp = nullptr;
    asm volatile("mov %%rbp, %0"
                 : "=r"(ebp));
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

    // disable the lock of console
    console::console_debugdisablelock();

    // change cga color and reset cursor to draw attention
    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);

    // panic message must be on the left-top of the console
    console::console_setpos(0);

    // first, print the given imformation.
    va_list ap;
    int c;

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
                console::printf("%c", va_arg(ap, int));
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

    constexpr size_t PCS_BUFLEN = 16;
    uintptr_t pcs[PCS_BUFLEN] = {0};
    kdebug_getcallerpcs(PCS_BUFLEN, pcs);

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

void kdebug::kdebug_panic2(const char *fmt, uint32_t topleft, ...)
{
    // disable interrupts
    cli();

    //TODO: disable the lock of console

    // change cga color and reset cursor to draw attention
    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);

    // panic2 message is usually on the left-top of the console
    if (topleft)
    {
        console::console_setpos(0);
    }

    // first, print the given imformation.
    va_list ap;
    int c;

    va_start(ap, topleft);

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
                console::printf("%c", va_arg(ap, int));
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

    constexpr size_t PCS_BUFLEN = 16;
    uintptr_t pcs[PCS_BUFLEN] = {0};
    kdebug_getcallerpcs(PCS_BUFLEN, pcs);

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