#include "sys/allocators/slab_alloc.h"
#include "sys/mm.h"

// slab
using allocators::slab_allocator::CACHE_NAME_MAXLEN;
using allocators::slab_allocator::SIZED_CACHE_COUNT;
using allocators::slab_allocator::slab;
using allocators::slab_allocator::slab_bufctl;
using allocators::slab_allocator::slab_cache;

using allocators::slab_allocator::slab_cache_alloc;
using allocators::slab_allocator::slab_cache_create;
using allocators::slab_allocator::slab_cache_destroy;
using allocators::slab_allocator::slab_cache_free;
using allocators::slab_allocator::slab_cache_reap;
using allocators::slab_allocator::slab_cache_shrink;
using allocators::slab_allocator::slab_init;

// defined in slab.cc
extern slab_cache *sized_caches[SIZED_CACHE_COUNT];
extern size_t sized_cache_count;

// buddy
using allocators::buddy_allocator::buddy_alloc_4k_page;
using allocators::buddy_allocator::buddy_alloc_with_order;
using allocators::buddy_allocator::buddy_free_4k_page;
using allocators::buddy_allocator::buddy_free_with_order;
using allocators::buddy_allocator::buddy_order_from_size;

constexpr size_t GUARDIAN_BYTES_AFTER = 16;

enum class allocator_types
{
    BUDDY,
    SLAB
};

struct memory_block
{
    allocator_types type;

    union {
        size_t size;  // used for slab
        size_t order; // used for buddy
    } size_info;

    uint8_t mem[0];
};

static inline slab_cache *cache_from_size(size_t sz)
{
    slab_cache *cache = nullptr;

    // assumption: sized_caches are sorted ascendingly.
    for (size_t i = 0; i < sized_cache_count; i++)
    {
        if (sz <= sized_caches[i]->obj_size)
        {
            cache = sized_caches[i];
            break;
        }
    }

    return cache;
}

void *memory::kmalloc(size_t sz, [[maybe_unused]] size_t flags)
{
    memory_block *ret = nullptr;

    size_t actual_size = sizeof(memory_block) + sz + GUARDIAN_BYTES_AFTER;

    if (actual_size > allocators::slab_allocator::BLOCK_SIZE)
    {
        // use buddy
        size_t order = buddy_order_from_size(actual_size);

        ret = reinterpret_cast<decltype(ret)>(buddy_alloc_with_order(order));

        ret->type = allocator_types::BUDDY;
        ret->size_info.order = order;
    }
    else
    {
        // use slab
        slab_cache *cache = cache_from_size(actual_size);

        ret = reinterpret_cast<decltype(ret)>(slab_cache_alloc(cache));
        ret->type = allocator_types::SLAB;
        ret->size_info.size = cache->obj_size;
    }

    return ret->mem;
}

void memory::kfree(void *ptr)
{
    uint8_t *mem_ptr = reinterpret_cast<decltype(mem_ptr)>(ptr);
    memory_block *block = reinterpret_cast<decltype(block)>(container_of(mem_ptr, memory_block, mem));

    if (block->type == allocator_types::BUDDY)
    {
        buddy_free_with_order(block, block->size_info.order);
    }
    else if (block->type == allocator_types::SLAB)
    {
        slab_cache *cache = cache_from_size(block->size_info.size);
        slab_cache_free(cache, block);
    }
}