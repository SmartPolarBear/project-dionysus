/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:01
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-09-24 23:19:08
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_ASM_H)
#define __INCLUDE_SYS_ASM_H

#if !defined(__cplusplus)
#define BOOT_STACK_SIZE (0x4000)
#define BOOT_STACK_ALIGN (0x1000)

#define KGDT_ENTRY (1)

#define GDT_ENTRY_SIZE 8

#define GDT_FLAG_4KB (1 << 3)
#define GDT_FLAG_32BITPM (1 << 2)
#define GDT_FLAG_64BITLM (1 << 1)

#define GDT_ACCESS_PRESENT (1 << 7)
#define GDT_ACCESS_PR_RING0 (0x0 << 5)
#define GDT_ACCESS_PR_RING1 (0x1 << 5)
#define GDT_ACCESS_PR_RING2 (0x2 << 5)
#define GDT_ACCESS_PR_RING3 (0x3 << 5)
#define GDT_ACCESS_EXECUTABLE (1 << 3)
#define GDT_ACCESS_DIRECTION_DOWN (1 << 2)
#define GDT_ACCESS_READABLE_WRITABLE (1 << 1)

#define SEG_NULLASM 0

#define SEG_ASM(base, limit, flags, access)           \
  (                                                   \
      (((((base)) >> 24) & 0xFF) << 56) |             \
      ((((flags)) & 0xF) << 52) |                     \
      (((((limit)) >> 16) & 0xF) << 48) |             \
      (((((access) | (1 << 4))) & 0xFF) << 40) |      \
      ((((base)) & 0xFFF) << 16) |                    \
      (((limit)) & 0xFFFF))


#define BOOTGHT_FIRST_ENTRY     SEG_NULLASM
#define BOOTGHT_SECOND_ENTRY    SEG_ASM(0,0,GDT_FLAG_64BITLM,GDT_ACCESS_PRESENT|GDT_ACCESS_PR_RING0|GDT_ACCESS_EXECUTABLE)
#else

#endif //__cplusplus

#endif // __INCLUDE_SYS_ASM_H
