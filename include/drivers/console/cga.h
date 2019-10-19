#if !defined(__INCLUDE_DRIVERS_CONSOLE_CGA_H)
#define __INCLUDE_DRIVERS_CONSOLE_CGA_H
#include "sys/types.h"
namespace console
{
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

static_assert(CGACOLOR_WHITE == 15);

void cga_putc(uint32_t c);
void cga_setpos(size_t pos);
void cga_setcolor(cga_colors fore, cga_colors bk);
} // namespace console

#endif // __INCLUDE_DRIVERS_CONSOLE_CGA_H
