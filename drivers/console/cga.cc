#include "drivers/console/cga.h"
#include "arch/amd64/x86.h"
#include "drivers/debug/kdebug.h"
#include "lib/libc/string.h"
#include "lib/libcxx/new"
#include "sys/memlayout.h"
#include "sys/types.h"

constexpr uint16_t backsp = 0x100;
constexpr uint16_t crt_port = 0x3d4;

static constexpr uint16_t MAKE_CGA_CHAR(char content, uint16_t attr)
{
    uint16_t ret = content | (attr << 8);
    return ret;
}

static constexpr uint16_t MAKE_CGA_ATTRIB(uint8_t freground, uint8_t background)
{
    return (background << 4) | (freground & 0x0F);
}

// global state for cga display style
struct
{
    uint8_t foreground = console::CGACOLOR_LIGHT_GREY;
    uint8_t background = console::CGACOLOR_BLACK;
} cga_attrib;

volatile uint16_t *cga_mem = (uint16_t *)(0xB8000 + KERNEL_VIRTUALBASE);

static size_t get_cur_pos(void)
{
    outb(crt_port, 14);
    size_t pos = inb(crt_port + 1) << 8;
    outb(crt_port, 15);
    pos |= inb(crt_port + 1);
    return pos;
}

static void set_cur_pos(size_t pos)
{
    outb(crt_port, 14);
    outb(crt_port + 1, pos >> 8);
    outb(crt_port, 15);
    outb(crt_port + 1, pos);
}

void console::cga_putc(uint32_t c)
{
    uint16_t attrib = MAKE_CGA_ATTRIB(cga_attrib.foreground, cga_attrib.background); //no background, lightgray foreground

    auto pos = get_cur_pos();

    if (c == '\n')
    {
        pos += 80 - pos % 80;
    }
    else if (c == backsp)
    {
        if (pos > 0)
        {
            --pos;
        }
    }
    else
    {
        cga_mem[pos++] = MAKE_CGA_CHAR(c, attrib);
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

    set_cur_pos(pos);
    cga_mem[pos] = MAKE_CGA_CHAR(' ', attrib);

    if (pos < 0 || pos > 25 * 80)
    {
        KDEBUG_GENERALPANIC(pos < 0
                                ? "pos out of bound: it should be positive."
                                : "pos out of bound: it should be smaller than 25*80");
    }
}

void console::cga_setpos(size_t pos)
{
    set_cur_pos(pos);
}

void console::cga_setcolor(cga_colors fore, cga_colors bk)
{
    cga_attrib.foreground = fore;
    cga_attrib.background = bk;
}