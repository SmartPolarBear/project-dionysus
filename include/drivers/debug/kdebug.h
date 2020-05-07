/*
 * Last Modified: Thu May 07 2020
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

#pragma once

#include "sys/types.h"

#include "drivers/lock/spinlock.h"

#include "drivers/debug/kerror.h"

#include "drivers/acpi/cpu.h"

namespace kdebug
{
extern bool panicked;

[[noreturn]] void kdebug_panic(const char *fmt, ...);
// use uint32_t for the bool value to make va_args happy.
[[noreturn]] void kdebug_panic2(const char *fmt, uint32_t topleft, ...);
[[noreturn]] void kdebug_dump_lock_panic(lock::spinlock *lock);

void kdebug_getcallerpcs(size_t buflen, uintptr_t pcs[]);

// panic with line number and file name
// to make __FILE__ and __LINE__ macros works right, this must be a macro as well.

#define KDEBUG_RICHPANIC(msg, title, topleft, add_fmt, args...) \
    kdebug::kdebug_panic2("[CPU%d]%s:\nIn file: %s, line: %d\nIn scope: %s\nMessage:\n%s\n" add_fmt, topleft, cpu->id, title, __FILE__, __LINE__, __PRETTY_FUNCTION__, msg, ##args)

#define KDEBUG_RICHPANIC_CODE(code, topleft, add_fmt, args...) \
    kdebug::kdebug_panic2("[CPU%d]%s:\nIn file: %s, line: %d\nIn scope: %s\nMessage:\n%s\n" add_fmt, topleft, cpu->id, kdebug::error_title(code), __FILE__, __LINE__, __PRETTY_FUNCTION__, kdebug::error_message(code), ##args)

#define KDEBUG_GERNERALPANIC_CODE(code) \
    KDEBUG_GENERALPANIC(kdebug::error_message(code))

#define KDEBUG_GENERALPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", true, "")

// pnaic that not move the cursor
#define KDEBUG_FOLLOWPANIC(msg) \
    KDEBUG_RICHPANIC(msg, "KDEBUG_GENERALPANIC", false, "")

// panic for not implemented functions
#define KDEBUG_NOT_IMPLEMENTED \
    KDEBUG_RICHPANIC("The function is not implemented", "KDEBUG_NOT_IMPLEMENTED", true, "");

// panic the kernel if the condition isn't equal to 1
#define KDEBUG_ASSERT(cond)                                                 \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            KDEBUG_RICHPANIC("Assertion failed",                            \
                             "ASSERT_PANIC",                                \
                             true,                                          \
                             "The expression \" %s \" is expected to be 1", \
                             #cond);                                        \
        }                                                                   \
    } while (0)

} // namespace kdebug
