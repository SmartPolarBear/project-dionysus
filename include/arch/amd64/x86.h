#if !defined(__INCLUDE_ARCH_X86_H)
#define __INCLUDE_ARCH_X86_H

#if !defined(__cplusplus)
#error Only available for C++
#endif

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
