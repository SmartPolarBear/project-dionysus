#include "drivers/console/cga.h"
#include "arch/amd64/x86.h"
#include "lib/libc/string.h"
#include "sys/memlayout.h"
#include "sys/types.h"

constexpr void *cga_addr = reinterpret_cast<void *>(0xB8000);
constexpr uint16_t cga_lightgray = 7;

constexpr uint16_t backsp = 0x100;
constexpr uint16_t crt_port = 0x3d4;

volatile uint16_t *cga_mem = new (P2V<void>(cga_addr)) uint16_t;

static size_t get_cur_pos(void)
{
    outb(crt_port, 14);
    size_t pos = inb(crt_port + 1) << 8;
    outb(crt_port, 15);
    pos |= inb(crt_port + 1);
    return pos;
}

static size_t set_cur_pos(size_t pos)
{
    outb(crt_port, 14);
    outb(crt_port + 1, pos >> 8);
    outb(crt_port, 15);
    outb(crt_port + 1, pos);
}

void console::cga_putc(char c)
{
    auto pos = get_cur_pos();
    uint16_t attrib = (0 << 4) | (cga_lightgray & 0x0F); //no background, lightgray foreground

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
        cga_mem[pos++] = c | (attrib << 8);
    }

    if (pos < 0 || pos > 25 * 80)
    {
        //TODO: panic kernel
    }

    if ((pos / 80) >= 24)
    {
        //scroll up the screen
        memmove((void *)(cga_mem),
                (void *)(cga_mem + 80),
                sizeof(cga_mem[0]) * 23 * 80);

        pos -= 80;
        memset((void *)(cga_mem), 0, sizeof(cga_mem[0]) * (24 + 80 - pos));
    }

    set_cur_pos(pos);
    cga_mem[pos] = ' ' | attrib;
}