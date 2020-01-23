/*
 * Last Modified: Fri Jan 24 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#if !defined(__INCLUDE_SYS_MM_H)
#define __INCLUDE_SYS_MM_H

#include "sys/types.h"

#include "sys/bootmm.h"
#include "sys/buddy_alloc.h"
#include "vm.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "lib/libcxx/algorithm"

extern uint8_t end[]; // kernel.ld

namespace vm
{

constexpr uintptr_t BOOT_MEM_LIMIT = 32_MB;

static inline void *kernel_boot_mem_begin(void)
{
    return end + multiboot::BOOT_INFO_MAX_EXPECTED_SIZE;
}

static inline void *kernel_boot_mem_end(void)
{
    return (void *)P2V(BOOT_MEM_LIMIT);
}

static inline void *kernel_mem_begin(void)
{
    // with a guard hole sized BOOTMM_BLOCKSIZE
    return reinterpret_cast<uint8_t *>(kernel_boot_mem_end()) + BOOTMM_BLOCKSIZE;
}

static inline void *kernel_mem_end(void)
{
    size_t phymem_size = get_physical_mem_size();
    if (phymem_size < KERNEL_SIZE)
    {
        return (void *)P2V(phymem_size);
    }
    else
    {
        return (void *)(VIRTUALADDR_LIMIT);
    }
}

} // namespace vm

#endif // __INCLUDE_SYS_MM_H
