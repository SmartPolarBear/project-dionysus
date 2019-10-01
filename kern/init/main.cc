/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-09-30 23:07:18
 * @ Description: the entry point for kernel in C++
 */

#include "sys/param.h"

#define VIDEO_START 0xb8000
#define VGA_LIGHT_GRAY 7

extern "C" int *mboot_ptr; //boot.S

static void
puts(const char *str)
{
    unsigned char *video = ((unsigned char *)VIDEO_START);
    while (*str != '\0')
    {
        *(video++) = *str++;
        *(video++) = VGA_LIGHT_GRAY;
    }
}

extern "C" [[noreturn]] void kmain() {
    puts("Hello World! fucker!");
    for (;;)
        ;
}
