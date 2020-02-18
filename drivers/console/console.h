#pragma once

#include "sys/types.h"

#include "lib/libkern/data/list.h"

using cursor_pos = size_t;
using char_attribute = size_t;


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


struct console_dev
{
    cursor_pos (*write_char)(uint32_t c, console_colors bk,console_colors fr);
    void (*set_cursor_pos)(cursor_pos pos);
    cursor_pos (*get_cursor_pos)(void);

    list_head dev_link;
};
