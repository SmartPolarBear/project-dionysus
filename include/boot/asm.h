/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:01
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-09-30 21:31:13
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_ASM_H)
#define __INCLUDE_SYS_ASM_H

#include "sys/cr.h"
#include "sys/gdt.h"
#include "sys/msr.h"

#if defined(__cplusplus)
#error Only allow for asm
#endif //__cplusplus

//kernel stack
#define BOOT_STACK_SIZE (0x4000)
#define BOOT_STACK_ALIGN (0x1000)

//gdt
#define SEG_NULLASM 0

#define SEG_ASM(base, limit, flags, access)      \
  (                                              \
      (((((base)) >> 24) & 0xFF) << 56) |        \
      ((((flags)) & 0xF) << 52) |                \
      (((((limit)) >> 16) & 0xF) << 48) |        \
      (((((access) | (1 << 4))) & 0xFF) << 40) | \
      ((((base)) & 0xFFF) << 16) |               \
      (((limit)) & 0xFFFF))

#define BOOTGDT_FIRST_ENTRY SEG_NULLASM
#define BOOTGDT_SECOND_ENTRY SEG_ASM(0, 0, GDT_FLAG_64BITLM, GDT_ACCESS_PRESENT | GDT_ACCESS_PR_RING0 | GDT_ACCESS_EXECUTABLE)

#define KGDT_ENTRY (1)

//cr0 value
#define BOOTCR0 (CR0_PAGING | CR0_PM | CR0_EXTTYPE)

//2mb paging
#define SHIFT_2MB (21)
#define VALUE_2MB (1 << SHIFT_2MB)

#endif // __INCLUDE_SYS_ASM_H
