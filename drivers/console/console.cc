#include "drivers/console/console.h"
#include "drivers/console/cga.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"

#include "sys/types.h"

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
    bool locking = conslock.lock_enable;
    if (locking)
    {
        spinlock_acquire(&conslock.lock);
    }

    while (*str != '\0')
    {
        console_putc_impl(*str);
        ++str;
    }

    // release the lock
    if (locking)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::putc(char c)
{
    // acquire the lock
    bool locking = conslock.lock_enable;
    if (locking)
    {
        spinlock_acquire(&conslock.lock);
    }

    console_putc_impl(c);

    // release the lock
    if (locking)
    {
        spinlock_release(&conslock.lock);
    }
}

// buffer for converting ints with itoa
constexpr size_t MAXNUMBER_LEN = 256;
char nbuf[MAXNUMBER_LEN] = {};

void console::vprintf(const char *fmt, va_list ap)
{
    // acquire the lock
    bool locking = conslock.lock_enable;
    if (locking)
    {
        spinlock_acquire(&conslock.lock);
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
            console_putc_impl(c);
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
            console_putc_impl(char_data(ch));
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
                console_putc_impl(nbuf[i]);
            }
            break;
        }
        case 'd':
        {
            size_t len = itoa_ex(nbuf, va_arg(ap, int), 10);
            for (size_t i = 0; i < len; i++)
            {
                console_putc_impl(nbuf[i]);
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
                    console_putc_impl(nbuf[i]);
                }
            }
            else
            {
                // Print unknown % sequence to draw attention.
                console_putc_impl('%');
                console_putc_impl('l');
                console_putc_impl(nextchars[0]);
                console_putc_impl(nextchars[1]);
            }
            break;
        }
        case 'x':
        {
            size_t len = itoa_ex(nbuf, va_arg(ap, int), 16);
            for (size_t i = 0; i < len; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        }
        case 'p':
        {
            static_assert(sizeof(size_t *) == sizeof(size_t));

            size_t len = itoa_ex(nbuf, va_arg(ap, size_t), 16);
            for (size_t i = 0; i < len; i++)
            {
                console_putc_impl(nbuf[i]);
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
                console_putc_impl(*s);
            }
            break;
        }
        case '%':
        {
            console_putc_impl('%');
            break;
        }
        default:
        {
            // Print unknown % sequence to draw attention.
            console_putc_impl('%');
            console_putc_impl(c);
            break;
        }
        }
    }

    // release the lock
    if (locking)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    
    console::vprintf(fmt, ap);

    va_end(ap);
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
    bool locking = conslock.lock_enable;
    if (locking)
    {
        spinlock_acquire(&conslock.lock);
    }

    console_setpos_impl(pos);

    // release the lock
    if (locking)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::console_settextattrib(size_t attribs)
{
    // acuqire the lock
    bool locking = conslock.lock_enable;

    if (locking)
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
    if (locking)
    {
        spinlock_release(&conslock.lock);
    }
}

void console::console_debugdisablelock(void)
{
    conslock.lock_enable = false;
}
