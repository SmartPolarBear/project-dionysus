#if !defined(__INCLUDE_ARCH_X86_H)
#define __INCLUDE_ARCH_X86_H

#if !defined(__cplusplus)
#error Only available for C++
#endif

static inline uint8_t inb(uint16_t port)
{
    uint8_t data = 0;
    asm volatile("in %1,%0"
                 : "=a"(data)
                 : "d"(port));

    return data;
}

static inline void outb(uint16_t port, uint8_t data)
{
    asm volatile("out %0,%1"
                 :
                 : "a"(data), "d"(port));
}

static inline void hlt(void)
{
    asm volatile("hlt");
}

static inline void cli(void)
{
    asm volatile("cli");
}

static inline void sti(void)
{
    asm volatile("sti");
}

#endif // __INCLUDE_ARCH_X86_H
