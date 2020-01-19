#include "sys/buddy_alloc.h"

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

using namespace vm;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

constexpr size_t MAX_ORD = 12;
constexpr size_t MIN_ORD = 6;

constexpr size_t ORD_COUNT = MAX_ORD - MIN_ORD + 1;
constexpr uint16_t MARK_EMPTY = 0xFFFF;

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

struct
{
    spinlock buddylock;
    uintptr_t start, start_mem;
    uintptr_t end;
    bool initialized;
    order orders[ORD_COUNT];
} buddy;

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

static inline void *block_id_to_memaddr(size_t order, size_t block_id)
{
    return (void *)(buddy.start_mem + (1 << order) * block_id);
}

static inline void block_set_available(size_t order, size_t blk_id)
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

static inline void block_set_unavailable(size_t order, size_t blk_id)
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
static inline void _buddy_free_impl(void *mem, size_t order)
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
        _buddy_free_impl(block_id_to_memaddr(order, block_id & ~0x0001), order + 1);
    }
}

static inline void _buddy_alloc()
{
    
}

static inline void _buddy_free(void *mem, size_t order)
{
    if ((order > MAX_ORD) || (order < MIN_ORD) || ((uintptr_t)mem) & ((1 << order) - 1))
    {
        KDEBUG_GENERALPANIC("_buddy_free: order out of range or memory unaligned\n");
    }

    spinlock_acquire(&buddy.buddylock);

    _buddy_free_impl(mem, order);

    spinlock_release(&buddy.buddylock);
}

void vm::buddy_init(void *st, void *ed)
{
    spinlock_initlock(&buddy.buddylock, "buddy_allocator");

    buddy.start = reinterpret_cast<uintptr_t>(st);
    buddy.end = reinterpret_cast<uintptr_t>(ed);

    size_t len = buddy.end - buddy.start;

    size_t n_marks = (len >> (ORD_COUNT + 5)) + 1, total_offset = 0;
    // can't use size_t because the end condition will never be reached
    for (int64_t i = ORD_COUNT - 1; i >= 0; i++)
    {
        order *ord = &buddy.orders[i];

        ord->head = MARK_EMPTY;
        ord->offset = total_offset;

        for (size_t j = 0; j < n_marks; j++)
        {
            auto mark = order_get_mark(i + MIN_ORD, j);
            mark->links = links(MARK_EMPTY, MARK_EMPTY);
            mark->bitmap = 0;
        }

        total_offset += n_marks;
        n_marks <<= 1;
    }

    buddy.start_mem = align_up(buddy.start + total_offset * sizeof(mark), 1 << ORD_COUNT);

    for (uintptr_t i = buddy.start_mem; i < buddy.end; i += (1 << ORD_COUNT))
    {
        _buddy_free((void *)i, ORD_COUNT);
    }

    buddy.initialized = true;
}

void *vm::buddy_alloc(size_t sz)
{
}

void vm::buddy_free(void *m)
{
}