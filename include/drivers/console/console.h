/*
 * Last Modified: Thu Feb 20 2020
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
#include "system/types.h"

namespace console
{

enum console_colors : uint8_t
{
    CONSOLE_COLOR_BLACK,
    CONSOLE_COLOR_BLUE,
    CONSOLE_COLOR_GREEN,
    CONSOLE_COLOR_CYAN,
    CONSOLE_COLOR_RED,
    CONSOLE_COLOR_MAGENTA,
    CONSOLE_COLOR_BROWN,
    CONSOLE_COLOR_LIGHT_GREY, // default color for foreground
    CONSOLE_COLOR_DARK_GREY,
    CONSOLE_COLOR_LIGHT_BLUE,
    CONSOLE_COLOR_LIGHT_GREEN,
    CONSOLE_COLOR_LIGHT_CYAN,
    CONSOLE_COLOR_LIGHT_RED,
    CONSOLE_COLOR_LIGHT_MAGENTA,
    CONSOLE_COLOR_LIGHT_BROWN, // looks like yellow
    CONSOLE_COLOR_WHITE
};

using cursor_pos = size_t;
using char_attribute = size_t;

void console_init();
void console_set_color(console_colors background, console_colors foreground);
void console_write_char(char c);

void cosnole_write_string(const char *str);
void cosnole_write_string(const char *str, size_t len);

void console_set_pos(cursor_pos pos);

void console_set_lock(bool enable);
bool console_get_lock(void);
} // namespace console

