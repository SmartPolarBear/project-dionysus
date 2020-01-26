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







#if !defined(__INCLUDE_DRIVERS_CONSOLE_H)
#define __INCLUDE_DRIVERS_CONSOLE_H
#include "sys/types.h"

namespace console
{
void printf(const char *fmt, ...);
void vprintf(const char *fmt, va_list args);
void putc(char c);
void puts(const char *str);
void console_init(void);

void console_setpos(size_t pos);
enum text_attributes
{
    //0

    TATTR_FRRED = 1 << 0,
    TATTR_FRGREEN = 1 << 1,
    TATTR_FRBLUE = 1 << 2,
    TATTR_FRYELLOW = 1 << 3,
    TATTR_FRMAGENTA = 1 << 4,
    TATTR_FRCYAN = 1 << 5,
    TATTR_FRLTGRAY = 1 << 6,
    TATTR_FRBLACK = 1 << 7,
    //8

    TATTR_BKRED = 1 << (0 + 8),
    TATTR_BKGREEN = 1 << (1 + 8),
    TATTR_BKBLUE = 1 << (2 + 8),
    TATTR_BKYELLOW = 1 << (3 + 8),
    TATTR_BKMAGENTA = 1 << (4 + 8),
    TATTR_BKCYAN = 1 << (5 + 8),
    TATTR_BKLTGRAY = 1 << (6 + 8),
    TATTR_BKBLACK = 1 << (7 + 8),

    //16
};

void console_settextattrib(size_t attribs);
void console_debugdisablelock(void);
} // namespace console

#endif // __INCLUDE_DRIVERS_CONSOLE_H
