#include "drivers/console/console.h"
#include "drivers/console/cga.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"

#include "sys/types.h"

#include <stdarg.h>

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

using putc_function_type = void (*)(uint32_t);
using setpos_function_type = void (*)(size_t);
using setcolor_function_type = void (*)(uint8_t fr, uint8_t bk);

const size_t COLORTABLE_LEN = 8;

struct console_dev
{
    putc_function_type putc;
    setpos_function_type setpos;
    setcolor_function_type setcolor;
    // R G B
    uint8_t colortable[16];
} console_devs[] = {
    [0] = {
        //CGA
        console::cga_putc,
        console::cga_setpos,
        reinterpret_cast<setcolor_function_type>(console::cga_setcolor),
        {[0] = console::CGACOLOR_RED,
         [1] = console::CGACOLOR_GREEN,
         [2] = console::CGACOLOR_BLUE,
         [3] = console::CGACOLOR_LIGHT_BROWN,
         [4] = console::CGACOLOR_MAGENTA,
         [5] = console::CGACOLOR_CYAN,
         [6] = console::CGACOLOR_LIGHT_GREY,
         [7] = console::CGACOLOR_BLACK}}};

static struct
{
    spinlock lock;
    bool lock_enable;
} conslock;

static inline void console_putc_impl(uint32_t c)
{
    for (auto dev : console_devs)
    {
        if (dev.putc)
        {
            dev.putc(c);
        }
    }
}

static inline void console_setpos_impl(size_t pos)
{
    for (auto dev : console_devs)
    {
        if (dev.setpos)
        {
            dev.setpos(pos);
        }
    }
}

static inline void console_setcolor_impl(uint8_t fridx, uint8_t bkidx)
{
    for (auto dev : console_devs)
    {
        if (dev.setcolor)
        {
            dev.setcolor(dev.colortable[fridx], dev.colortable[bkidx]);
        }
    }
}

void console::puts(const char *str)
{
    // acquire the lock
    if (conslock.lock_enable)
    {
        spinlock_acquire(&conslock.lock);
    }

    while (*str != '\0')
    {
        console_putc_impl(*str);
        ++str;
    }

    // release the lock
    if (conslock.lock_enable)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::putc(char c)
{
    // acquire the lock
    if (conslock.lock_enable)
    {
        spinlock_acquire(&conslock.lock);
    }

    console_putc_impl(c);

    // release the lock
    if (conslock.lock_enable)
    {
        spinlock_release(&conslock.lock);
    }
}

// buffer for converting ints with itoa
constexpr size_t MAXNUMBER_LEN = 128;
char nbuf[MAXNUMBER_LEN] = {};

void console::printf(const char *fmt, ...)
{
    va_list ap;
    int i, c; //, locking;
    char ch = 0;
    const char *s;

    va_start(ap, fmt);

    // acquire the lock
    if (conslock.lock_enable)
    {
        spinlock_acquire(&conslock.lock);
    }

    if (fmt == 0)
    {
        KDEBUG_GENERALPANIC("Invalid format strings");
    }

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
    {
        if (c != '%')
        {
            console_putc_impl(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c)
        {
        case 'c':
            // this va_arg uses int
            // otherwise, a warning will be given, saying
            // warning: second argument to 'va_arg' is of promotable type 'char'; this va_arg has undefined behavior because arguments will be promoted to 'int
            ch = va_arg(ap, int);
            console_putc_impl(ch & 0xFF);
        case 'd':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 10);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 'x':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 'p':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, size_t), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                console_putc_impl(*s);
            break;
        case '%':
            console_putc_impl('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            console_putc_impl('%');
            console_putc_impl(c);
            break;
        }
    }

    va_end(ap);

    // release the lock
    if (conslock.lock_enable)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::console_init(void)
{
    spinlock_initlock(&conslock.lock, "console");
    console_setpos(0);
    conslock.lock_enable = true;
}

void console::console_setpos(size_t pos)
{
    // acuqire the lock
    if (conslock.lock_enable)
    {
        spinlock_acquire(&conslock.lock);
    }

    console_setpos_impl(pos);

    // release the lock
    if (conslock.lock_enable)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::console_settextattrib(size_t attribs)
{
    // acuqire the lock
    if (conslock.lock_enable)
    {
        spinlock_acquire(&conslock.lock);
    }

    // color flags are ranged from 1<<0 to 1<<(COLORTABLE_LEN-1)
    if (((SIZE_MAX >> (64 - COLORTABLE_LEN)) << 0) & attribs) //check if any color flags
    {
        uint8_t fridx = 0, bkidx = 0;
        for (uint8_t i = 0;
             i < uint8_t(COLORTABLE_LEN);
             i++) // from 0 to COLORTABLE_LEN, test foreground
        {
            if ((1 << i) & attribs) // find first foreground bit
            {
                fridx = i;
                break;
            }
        }

        for (uint8_t i = uint8_t(COLORTABLE_LEN);
             i < uint8_t(COLORTABLE_LEN) + uint8_t(COLORTABLE_LEN);
             i++) // from COLORTABLE_LEN to 2*COLORTABLE_LEN-1, test background
        {
            if ((1 << i) & attribs) //find first background bit
            {
                bkidx = i;
                break;
            }
        }

        // set color. the background is within [COLORTABLE_LEN,2*COLORTABLE_LEN)
        // so the bkidex modulo COLORTABLE_LEN should be with in [0,COLORTABLE_LEN)
        // and therefore be valid for a index to color tables
        console_setcolor_impl(fridx % COLORTABLE_LEN, bkidx % COLORTABLE_LEN);
    }

    // release the lock
    if (conslock.lock_enable)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::console_debugdisablelock(void)
{
    conslock.lock_enable = false;
}
