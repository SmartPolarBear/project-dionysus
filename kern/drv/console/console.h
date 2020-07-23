#pragma once

#include "system/types.h"

#include "libraries/libkernel/data/List.h"

#include "drivers/console/console.h"

using console::console_colors;
using console::cursor_pos;

struct console_dev
{
    cursor_pos (*write_char)(uint32_t c, console_colors bk, console_colors fr);
    void (*set_cursor_pos)(cursor_pos pos);
    cursor_pos (*get_cursor_pos)(void);

    list_head dev_link;
};
