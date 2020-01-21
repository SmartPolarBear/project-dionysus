/*
 * Last Modified: Mon Jan 20 2020
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

#if !defined(__KERN_MM_ALLOCATORS_BUDDY_BUDDY_H)
#define __KERN_MM_ALLOCATORS_BUDDY_BUDDY_H

#include "sys/types.h"

#include "drivers/lock/spinlock.h"

namespace buddy_internal
{

// For the sake of access, constants and types are encapsuled in two child namesapce
// so that they can be easily introduce by simply using namesapce

namespace constants
{
constexpr size_t MAX_ORD = 12;
constexpr size_t MIN_ORD = 6;

constexpr size_t ORD_COUNT = MAX_ORD - MIN_ORD + 1;
constexpr uint16_t MARK_EMPTY = 0xFFFF;

} // namespace constants

using namespace constants;

namespace types
{
struct mark
{
    size_t links;
    size_t bitmap;
};

struct order
{
    size_t head;
    size_t offset;
};

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

struct buddy_struct
{
    spinlock buddylock;
    uintptr_t start, start_mem;
    uintptr_t end;
    bool initialized;
    order orders[ORD_COUNT];
};

using raw_ptr = void *;
} // namespace types

using namespace types;

// buddy.cc
extern buddy_struct buddy;

static inline mark *order_get_mark(size_t order, size_t idx)
{
    return reinterpret_cast<mark *>(buddy.start + (buddy.orders[order - MIN_ORD].offset + idx));
}

static inline constexpr size_t align_up(size_t sz, size_t al)
{
    return (((size_t)(sz) + (size_t)(al)-1) & ~((size_t)(al)-1));
}

static inline constexpr size_t align_down(size_t sz, size_t al)
{
    return ((size_t)(sz) & ~((size_t)(al)-1));
}

static inline constexpr size_t link_pre(size_t links)
{
    return links >> 16;
}

static inline constexpr size_t link_next(size_t links)
{
    return links & 0xFFFF;
}

static inline constexpr size_t links(size_t pre, size_t next)
{
    return ((pre << 16) | (next & 0xFFFF));
}

static inline size_t mem_get_block_id(size_t order, uintptr_t mem)
{
    return (mem - buddy.start_mem) >> order;
}

static inline size_t mark_if_available(const mark *mk, size_t blk_id)
{
    return mk->bitmap & (1 << (blk_id & 0x1F));
}

static inline raw_ptr block_id_to_memaddr(size_t order, size_t block_id)
{
    return reinterpret_cast<raw_ptr>(buddy.start_mem + (1 << order) * block_id);
}

// buddy_utils.cc
void block_set_available(size_t order, size_t blk_id);
void block_set_unavailable(size_t order, size_t blk_id);
raw_ptr order_get_block(size_t order);

// buddy_impl.cc
raw_ptr buddy_alloc(size_t order);
void buddy_free(raw_ptr mem, size_t order);

// these shouldn't be called directly, but is included for the sake of access to things in this namespace
void buddy_free_impl(raw_ptr mem, size_t order);
raw_ptr buddy_alloc_impl(size_t order);

} // namespace buddy_internal

#endif // __KERN_MM_ALLOCATORS_BUDDY_BUDDY_H
