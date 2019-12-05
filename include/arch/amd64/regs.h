#if !defined(__INCLUDE_ARCH_AMD64_REGS_H)
#define __INCLUDE_ARCH_AMD64_REGS_H

#include "sys/types.h"

static inline uintptr_t rcr2(void)
{
    uintptr_t val;
    asm volatile("mov %%cr2,%0"
                 : "=r"(val));
    return val;
}

static inline void lcr3(uintptr_t val)
{
    asm volatile("mov %0,%%cr3"
                 :
                 : "r"(val));
}

//read eflags
static inline size_t read_eflags(void)
{
    size_t eflags = 0;
    asm volatile("pushf; pop %0"
                 : "=r"(eflags));
    return eflags;
}

enum eflags_value
{
    // Eflags register
    EFLAG_CF = 0x00000001,        // Carry Flag
    EFLAG_PF = 0x00000004,        // Parity Flag
    EFLAG_AF = 0x00000010,        // Auxiliary carry Flag
    EFLAG_ZF = 0x00000040,        // Zero Flag
    EFLAG_SF = 0x00000080,        // Sign Flag
    EFLAG_TF = 0x00000100,        // Trap Flag
    EFLAG_IF = 0x00000200,        // Interrupt Enable
    EFLAG_DF = 0x00000400,        // Direction Flag
    EFLAG_OF = 0x00000800,        // Overflow Flag
    EFLAG_IOPL_MASK = 0x00003000, // I/O Privilege Level bitmask
    EFLAG_IOPL_0 = 0x00000000,    //   IOPL == 0
    EFLAG_IOPL_1 = 0x00001000,    //   IOPL == 1
    EFLAG_IOPL_2 = 0x00002000,    //   IOPL == 2
    EFLAG_IOPL_3 = 0x00003000,    //   IOPL == 3
    EFLAG_NT = 0x00004000,        // Nested Task
    EFLAG_RF = 0x00010000,        // Resume Flag
    EFLAG_VM = 0x00020000,        // Virtual 8086 mode
    EFLAG_AC = 0x00040000,        // Alignment Check
    EFLAG_VIF = 0x00080000,       // Virtual Interrupt Flag
    EFLAG_VIP = 0x00100000,       // Virtual Interrupt Pending
    EFLAG_ID = 0x00200000,        // ID flag
};



#endif // __INCLUDE_ARCH_AMD64_REGS_H
