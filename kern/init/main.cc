/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-23 23:06:29
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-01 15:21:57
 * @ Description: the entry point for kernel in C++
 */

#include "boot/multiboot2.h"
#include "sys/param.h"

#define VIDEO_START 0xb8000
#define VGA_LIGHT_GRAY 7

extern "C" void *mboot_addr; //boot.S

struct mboot_info
{
};

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

//C++ ctors and dtors
extern "C"
{
    using constructor_type = void (*)();
    extern constructor_type start_ctors;
    extern constructor_type end_ctors;

    void call_ctors(void)
    {
        for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
        {
            (*ctor)();
        }
    }
}

extern "C" [[noreturn]] void kmain() {
    puts("Hello World! fucker!");
    for (;;)
        ;
}
