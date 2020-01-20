#if !defined(__KERN_MM_ALLOCATORS_BUDDY_BUDDY_H)
#define __KERN_MM_ALLOCATORS_BUDDY_BUDDY_H

#include "sys/types.h"

#include "drivers/lock/spinlock.h"

namespace buddy_internal
{

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

static inline void *block_id_to_memaddr(size_t order, size_t block_id)
{
    return (void *)(buddy.start_mem + (1 << order) * block_id);
}

void block_set_available(size_t order, size_t blk_id);
void block_set_unavailable(size_t order, size_t blk_id);
void buddy_alloc();
void buddy_free_impl(void *mem, size_t order);
void buddy_free(void *mem, size_t order);

} // namespace buddy_internals

#endif // __KERN_MM_ALLOCATORS_BUDDY_BUDDY_H
