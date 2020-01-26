/*
 * Last Modified: Sat Jan 25 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */







#include "arch/amd64/x86.h"

#include "drivers/debug/kdebug.h"
#include "drivers/console/console.h"

#include "sys/memlayout.h"
#include "sys/types.h"

bool kdebug::panicked = false;

static inline void kdebug_vpanic_print_impl(const char *fmt, bool topleft, va_list ap)
{
    // disable the lock of console
    console::console_debugdisablelock();

    // change cga color and reset cursor to draw attention
    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);

    // panic2 message is usually on the left-top of the console
    if (topleft)
    {
        console::console_setpos(0);
    }

    console::vprintf(fmt, ap);

    constexpr size_t PCS_BUFLEN = 16;
    uintptr_t pcs[PCS_BUFLEN] = {0};
    kdebug::kdebug_getcallerpcs(PCS_BUFLEN, pcs);

    console::printf("\n");
    for (auto pc : pcs)
    {
        console::printf("%p ", pc);
    }
}

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

[[noreturn]] void kdebug::kdebug_panic(const char *fmt, ...)
{
    // disable interrupts
    cli();

    va_list ap;
    va_start(ap, fmt);

    kdebug_vpanic_print_impl(fmt, false, ap);

    va_end(ap);

    // set global panic state for other cpu
    panicked = true;

    // infinite loop
    for (;;)
        ;
}

[[noreturn]] void kdebug::kdebug_panic2(const char *fmt, uint32_t topleft, ...)
{
    // disable interrupts
    cli();

    va_list ap;
    va_start(ap, topleft);

    kdebug_vpanic_print_impl(fmt, topleft, ap);

    va_end(ap);

    // set global panic state for other cpu
    panicked = true;

    // infinite loop
    for (;;)
        ;
}

void kdebug::kdebug_dump_lock_panic(lock::spinlock *lock)
{
    // disable the lock of console
    console::console_debugdisablelock();
    console::console_setpos(0);

    constexpr auto panicked_screencolor = console::TATTR_BKBLUE | console::TATTR_FRYELLOW;
    console::console_settextattrib(panicked_screencolor);

    console::printf("lock %s has been held.\nCall stack:\n", lock->name);

    for (auto cs : lock->pcs)
    {
        console::printf("%p ", cs);
    }

    console::printf("\n");

    KDEBUG_FOLLOWPANIC("acquire");
}
