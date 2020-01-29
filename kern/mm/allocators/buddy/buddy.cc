/*
 * Last Modified: Wed Jan 29 2020
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

#include "sys/allocators/buddy_alloc.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "lib/libc/string.h"

#include "./buddy.h"

using namespace buddy_internal::types;
using namespace buddy_internal::constants;

constexpr size_t ORDER_4K = 12;

buddy_struct buddy_internal::buddy = {};

using buddy_internal::buddy;

void allocators::buddy_allocator::buddy_init(raw_ptr st, raw_ptr ed)
{
    spinlock_initlock(&buddy.buddylock, "buddy_allocator");

    buddy.start = PAGE_ROUNDUP(reinterpret_cast<uintptr_t>(st));
    buddy.end = PAGE_ROUNDDOWN(reinterpret_cast<uintptr_t>(ed));

    size_t len = buddy.end - buddy.start;

    size_t n_marks = (len >> (MAX_ORD + 5)) + 1;

    size_t total_offset = 0;

    // can't use size_t here because the end condition will never be reached
    // use int because ORD_COUNT won't be bigger than INT_MAX
    for (int i = ORD_COUNT - 1; i >= 0; i--)
    {
        order *ord = &buddy.orders[i];

        ord->head = MARK_EMPTY;
        ord->offset = total_offset;

        for (size_t j = 0; j < n_marks; j++)
        {
            auto mark = buddy_internal::order_get_mark(i + MIN_ORD, j);
            mark->links = buddy_internal::links(MARK_EMPTY, MARK_EMPTY);
            mark->bitmap = 0;
        }

        total_offset += n_marks;
        n_marks <<= 1;
    }

    buddy.start_mem = buddy_internal::align_up(buddy.start + total_offset * sizeof(mark), 1 << MAX_ORD);

    for (uintptr_t i = buddy.start_mem; i < buddy.end; i += (1 << MAX_ORD))
    {
        buddy_internal::buddy_free(reinterpret_cast<raw_ptr>(i), MAX_ORD);
    }

    buddy.initialized = true;
    buddy.lock_enable = true;
}


void *allocators::buddy_allocator::buddy_alloc_4k_page(void)
{
    // the order of a 4K page (4096 byte) is 12
    return buddy_alloc_with_order(ORDER_4K);
}

void allocators::buddy_allocator::buddy_free_4k_page(void* ptr)
{
    buddy_free_with_order(ptr, ORDER_4K);
}

void *allocators::buddy_allocator::buddy_alloc_with_order(size_t order)
{
    KDEBUG_ASSERT(order >= MIN_ORD && order <= MAX_ORD);
    return buddy_internal::buddy_alloc(order);
}

void allocators::buddy_allocator::buddy_free_with_order(void *ptr,size_t order)
{
    buddy_internal::buddy_free(ptr,order);
}

size_t allocators::buddy_allocator::buddy_order_from_size(size_t bytes)
{
    return buddy_internal::size_to_order(bytes);
}

