
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
        KDEBUG_GENERALPANIC("buddy: freeing a free block.");
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

// seperate from _buddy_free for the sake of recursive calls 
void buddy_internal::buddy_free_impl(void *mem, size_t order)
{
    size_t block_id = mem_get_block_id(order, reinterpret_cast<uintptr_t>(mem));
    mark *mark = order_get_mark(order, block_id >> 5);

    if (mark_if_available(mark, block_id))
    {
        KDEBUG_GENERALPANIC("buddy: freeing a free block.");
    }

    size_t buddy_id = block_id ^ 0x0001; // blk_id and buddy_id differs in the last bit
                                         // buddy must be in the same bit map

    if (!mark_if_available(mark, buddy_id) || (order == ORD_COUNT))
    {
        block_set_available(order, block_id);
    }
    else
    {
        block_set_unavailable(order, buddy_id);
        buddy_free_impl(block_id_to_memaddr(order, block_id & ~0x0001), order + 1);
    }
}

void buddy_internal::buddy_alloc()
{
    
}

void buddy_internal::buddy_free(void *mem, size_t order)
{
    if ((order > MAX_ORD) || (order < MIN_ORD) || ((uintptr_t)mem) & ((1 << order) - 1))
    {
        KDEBUG_GENERALPANIC("_buddy_free: order out of range or memory unaligned\n");
    }

    spinlock_acquire(&buddy.buddylock);

    buddy_free_impl(mem, order);

    spinlock_release(&buddy.buddylock);
}