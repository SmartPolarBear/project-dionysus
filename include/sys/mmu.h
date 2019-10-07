/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-07 14:45:09
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_MMU_H)
#define __INCLUDE_SYS_MMU_H

#if !defined(__cplusplus)
#define PML4_SIZE (0x1000)
#define PML4_ALIGN (0x1000)
#define PML4_ENTRY_SIZE (8)
#define PML4_ADDR2ENTRYID(addr) (((addr) >> 39) & 0x1FF)

#define PDPT_SIZE (0x1000)
#define PDPT_ALIGN (0x1000)
#define PDPT_ENTRY_SIZE (8)
#define PDPT_ADDR2ENTRYID(addr) (((addr) >> 30) & 0x1FF)

#define PD_SIZE (0x1000)
#define PD_ALIGN (0x1000)
#define PD_ENTRY_SIZE (8)

//Unused: we use 2MB page
#define PT_SIZE (0x1000)
#define PT_ALIGN (0x1000)
#define PT_ENTRY_SIZE (8)

#define PG_P (1 << 0)
#define PG_W (1 << 1)
#define PG_U (1 << 2)
#define PG_2MB (1 << 7)
#else

#include "sys/types.h"

constexpr size_t PML4T_SIZE = 0x1000;
constexpr size_t PML4T_ALIGN = 0x1000;
constexpr size_t PML4T_ENTRY_SIZE = 8;

constexpr size_t PDPT_SIZE = 0x1000;
constexpr size_t PDPT_ALIGN = 0x1000;
constexpr size_t PDPT_ENTRY_SIZE = 8;

constexpr size_t PD_SIZE = 0x1000;
constexpr size_t PD_ALIGN = 0x1000;
constexpr size_t PD_ENTRY_SIZE = 8;

constexpr size_t PG_P = (1 << 0);  //Present
constexpr size_t PG_W = (1 << 1);  //Writable
constexpr size_t PG_U = (1 << 2);  //User
constexpr size_t PG_PS = (1 << 7); //2MB pages

constexpr size_t PML4_BASE(uintptr_t base) { (((uint64_t)(base) >> 39) & 0x1FF); }
constexpr size_t PDPT_BASE(uintptr_t base) { (((uint64_t)(base) >> 30) & 0x1FF); }
constexpr size_t PDIR_BASE(uintptr_t base) { (((uint64_t)(base) >> 21) & 0x1FF); }
constexpr size_t PTABLE_BASE(uintptr_t base) { (((uint64_t)(base) >> 12) & 0x1FF); }
constexpr uintptr_t PTE_ADDR(uintptr_t pte)
{
    return ((size_t)(pte) & ~0xFFF);
}


//As we don't use 4K pages, we have no need to define things for PTE here.

#endif //__cplusplus

#endif // __INCLUDE_SYS_MMU_H
