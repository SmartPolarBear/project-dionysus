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

void buddy_internal::block_set_available(size_t order, size_t blk_id)
{
    auto ord = &buddy.orders[order - MIN_ORD];
    auto mrk = order_get_mark(order, blk_id >> 5);

    bool need_insert = mrk->bitmap == 0;

    if (mark_if_available(mrk, blk_id))
    {
        KDEBUG_GENERALPANIC("block_set_available: freeing a free block.");
    }

    mrk->bitmap |= (1 << (blk_id & 0x1F));

    if (need_insert)
    {
        blk_id >>= 5;
        mrk->links = links(MARK_EMPTY, ord->head);

        if (ord->head != MARK_EMPTY)
        {
            auto pre = order_get_mark(order, ord->head);
            pre->links = links(blk_id, link_next(pre->links));
        }

        ord->head = blk_id;
    }
}

void buddy_internal::block_set_unavailable(size_t order, size_t blk_id)
{
    auto ord = &buddy.orders[order - MIN_ORD];
    auto mrk = order_get_mark(order, blk_id >> 5);

    if (!mark_if_available(mrk, blk_id))
    {
        KDEBUG_GENERALPANIC("buddy: alloc twice.");
    }

    mrk->bitmap &= ~(1 << (blk_id & 0x1F));

    if (mrk->bitmap == 0)
    {
        blk_id >>= 5;

        auto prev = link_pre(mrk->links);
        auto next = link_next(mrk->links);

        if (prev != MARK_EMPTY)
        {
            auto p = order_get_mark(order, prev);
            p->links = links(link_pre(p->links), next);
        }
        else if (ord->head == blk_id)
        {
            // if we are the first in the link
            ord->head = next;
        }

        if (next != MARK_EMPTY)
        {
            auto p = order_get_mark(order, next);
            p->links = links(prev, link_next(p->links));
        }

        mrk->links = links(MARK_EMPTY, MARK_EMPTY);
    }
}

buddy_internal::raw_ptr buddy_internal::order_get_block(size_t order)
{
    auto ord = &buddy.orders[order - MIN_ORD];
    auto mrk = order_get_mark(order, ord->head);

    if (mrk->bitmap == 0)
    {
        KDEBUG_GENERALPANIC("order_get_block: empty mark in the list");
    }

    for (size_t i = 0; i < 32; i++)
    {
        if (mrk->bitmap & (1 << i))
        {
            auto blk_id = ord->head * 32 + i;
            block_set_unavailable(order, blk_id);

            return block_id_to_memaddr(order, blk_id);
        }
    }

    return nullptr;
}
