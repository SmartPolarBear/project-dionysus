#if !defined(__INCLUDE_DRIVERS_CONSOLE_H)
#define __INCLUDE_DRIVERS_CONSOLE_H
#include "sys/types.h"
namespace console
{
void printf(const char *fmt, ...);
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
    TATTR_FRBBLACK = 1 << 7,
    //8

    TATTR_BKRED = 1 << (0 + 8),
    TATTR_BKGREEN = 1 << (1 + 8),
    TATTR_BKBLUE = 1 << (2 + 8),
    TATTR_BKYELLOW = 1 << (3 + 8),
    TATTR_BKMAGENTA = 1 << (4 + 8),
    TATTR_BKCYAN = 1 << (5 + 8),
    TATTR_BKLTGRAY = 1 << (6 + 8),
    TATTR_BKBBLACK = 1 << (7 + 8),

    //16
};

void console_settextattrib(size_t attribs);
} // namespace console

#endif // __INCLUDE_DRIVERS_CONSOLE_H
