/*
 * Last Modified: Thu Jan 23 2020
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

#include "sys/buddy_alloc.h"

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "./buddy.h"

buddy_internal::raw_ptr buddy_internal::buddy_alloc_impl(size_t order)
{
    if (!buddy.initialized)
    {
        KDEBUG_GENERALPANIC("buddy_alloc_impl: cannot alloc before panic");
    }

    auto ord = &buddy.orders[order - MIN_ORD];
    raw_ptr ret = nullptr;

    if (ord->head != MARK_EMPTY)
    {
        ret = order_get_block(order);
    }
    else if (order < MAX_ORD)
    {
        ret = buddy_alloc_impl(order + 1);

        if (ret != nullptr)
        {
            // make the compiler happy.
            uint8_t *typed_ret = reinterpret_cast<decltype(typed_ret)>(ret);
            buddy_free_impl(typed_ret + (1 << order), order);
        }
    }

    return ret;
}

// seperate from _buddy_free for the sake of recursive calls
void buddy_internal::buddy_free_impl(raw_ptr mem, size_t order)
{
    size_t block_id = mem_get_block_id(order, (uintptr_t)(mem));
    mark *mark = order_get_mark(order, block_id >> 5);

    if (mark_if_available(mark, block_id))
    {
        KDEBUG_GENERALPANIC("buddy_free_impl: freeing a free block.");
    }

    size_t buddy_id = block_id ^ 0x0001; // blk_id and buddy_id differs in the last bit
                                         // buddy must be in the same bit map

    if (!mark_if_available(mark, buddy_id) || (order == MAX_ORD))
    {
        block_set_available(order, block_id);
    }
    else
    {
        block_set_unavailable(order, buddy_id);
        buddy_free_impl(block_id_to_memaddr(order, block_id & ~0x0001), order + 1);
    }
}

buddy_internal::raw_ptr buddy_internal::buddy_alloc(size_t order)
{
    if (order > MAX_ORD || order < MIN_ORD)
    {
        KDEBUG_GENERALPANIC("buddy_alloc: given order is out of range.");
    }

    spinlock_acquire(&buddy.buddylock);

    auto ret = buddy_alloc_impl(order);

    spinlock_release(&buddy.buddylock);

    return ret;
}

void buddy_internal::buddy_free(raw_ptr mem, size_t order)
{
    if ((order > MAX_ORD) || (order < MIN_ORD))
    {
        KDEBUG_GENERALPANIC("buddy_free: order out of range or memory unaligned\n");
    }
    else if (((uintptr_t)mem) & ((1 << order) - 1))
    {
        KDEBUG_GENERALPANIC("buddy_free: memory unaligned\n");
    }

    spinlock_acquire(&buddy.buddylock);

    buddy_free_impl(mem, order);

    spinlock_release(&buddy.buddylock);
}