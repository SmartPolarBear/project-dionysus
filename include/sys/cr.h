/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-24 23:22:18
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-09-30 21:36:05
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_CR_H)
#define __INCLUDE_SYS_CR_H

#if !defined(__cplusplus)
#define CR0_PM (1 << 0)
#define CR0_EXTTYPE (1 << 4)
#define CR0_PAGING (1 << 31)

#define CR4_PSE (1 << 4)
#define CR4_PAE (1 << 5)
#else
#endif

#endif // __INCLUDE_SYS_CR_H
