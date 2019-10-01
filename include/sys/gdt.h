#if !defined(__INCLUDE_SYS_GDT_H)
#define __INCLUDE_SYS_GDT_H

#if !defined(__cplusplus)
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
#else

#endif

#endif // __INCLUDE_SYS_GDT_H
