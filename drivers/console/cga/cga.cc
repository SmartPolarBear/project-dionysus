#include "cga.h"

#include "arch/amd64/x86.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"

#include "sys/memlayout.h"
#include "sys/types.h"

enum cga_colors : uint8_t
{
    CGACOLOR_BLACK,
    CGACOLOR_BLUE,
    CGACOLOR_GREEN,
    CGACOLOR_CYAN,
    CGACOLOR_RED,
    CGACOLOR_MAGENTA,
    CGACOLOR_BROWN,
    CGACOLOR_LIGHT_GREY, // default color for foreground
    CGACOLOR_DARK_GREY,
    CGACOLOR_LIGHT_BLUE,
    CGACOLOR_LIGHT_GREEN,
    CGACOLOR_LIGHT_CYAN,
    CGACOLOR_LIGHT_RED,
    CGACOLOR_LIGHT_MAGENTA,
    CGACOLOR_LIGHT_BROWN, // looks like yellow
    CGACOLOR_WHITE
};

volatile uint16_t *cga_mem = (uint16_t *)(0xB8000 + KERNEL_VIRTUALBASE);

constexpr uint16_t KEYCODE_BACKSPACE = 0x100;
constexpr uint16_t CRT_PORT = 0x3d4;

console_dev cga_dev
{
    .write_char=cga_write_char,
    .set_cursor_pos=set_cursor_pos,
    .get_cursor_pos=get_cursor_pos
};

static constexpr uint16_t make_cga_char(char content, uint16_t attr)
{
    uint16_t ret = content | (attr << 8);
    return ret;
}

static constexpr uint16_t make_cga_color_attrib(uint8_t freground, uint8_t background)
{
    return (background << 4) | (freground & 0x0F);
}

cursor_pos get_cursor_pos(void)
{
    outb(CRT_PORT, 14);
    size_t pos = inb(CRT_PORT + 1) << 8;
    outb(CRT_PORT, 15);
    pos |= inb(CRT_PORT + 1);
    return pos;
}

void set_cursor_pos(cursor_pos pos)
{
    outb(CRT_PORT, 14);
    outb(CRT_PORT + 1, pos >> 8);
    outb(CRT_PORT, 15);
    outb(CRT_PORT + 1, pos);
}

cursor_pos cga_write_char(uint32_t c, console_colors bk,console_colors fr)
{
    uint16_t attrib = make_cga_color_attrib(fr, bk); //no background, lightgray foreground

    auto pos = get_cursor_pos();

    if (c == '\n')
    {
        pos += 80 - pos % 80;
    }
    else if (c == KEYCODE_BACKSPACE)
    {
        if (pos > 0)
        {
            --pos;
        }
    }
    else
    {
        cga_mem[pos++] = make_cga_char(c, attrib);
    }

    if ((pos / 80) >= 24)
    {
        //scroll up the screen
        memmove((void *)(cga_mem),
                (void *)(cga_mem + 80),
                sizeof(cga_mem[0]) * 23 * 80);

        pos -= 80;
        memset((void *)(cga_mem + pos), 0, sizeof(cga_mem[0]) * (24 * 80 - pos));
    }

    set_cursor_pos(pos);
    cga_mem[pos] = make_cga_char(' ', attrib);

    if (pos < 0 || pos > 25 * 80)
    {
        auto msg = pos < 0
                       ? "pos out of bound: it should be positive."
                       : "pos out of bound: it should be smaller than 25*80";

        KDEBUG_RICHPANIC(msg,
                             "KERNEL PANIC: BUILTIN CGA",
                             false,
                             "The pos now is %d", pos);
    }

    return pos;
}