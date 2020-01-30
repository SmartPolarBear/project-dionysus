/*
 * Last Modified: Thu Jan 30 2020
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
#include "sys/allocators/slab_alloc.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"

// slab
using allocators::slab_allocator::BLOCK_SIZE;
using allocators::slab_allocator::CACHE_NAME_MAXLEN;
using allocators::slab_allocator::SIZED_CACHE_COUNT;
using allocators::slab_allocator::slab;
using allocators::slab_allocator::slab_bufctl;
using allocators::slab_allocator::slab_cache;

// buddy
using allocators::buddy_allocator::buddy_alloc_with_order;
using allocators::buddy_allocator::buddy_free_with_order;

using allocators::buddy_allocator::buddy_alloc_4k_page;
using allocators::buddy_allocator::buddy_free_4k_page;

// linked list
using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

//ATTENTION: IF ANY WEIRD BUG OCCURS IN THIS FILE, CONSIDER RACE CONDITION FIRST!
// spinlock
using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_holding;
using lock::spinlock_initlock;
using lock::spinlock_release;

list_head cache_head;
spinlock cache_head_lock;

slab_cache *sized_caches[SIZED_CACHE_COUNT];

slab_cache cache_cache;

static inline constexpr size_t cache_obj_count(size_t obj_size)
{
    return BLOCK_SIZE / (sizeof(slab_bufctl) + obj_size);
}

static inline void *slab_cache_grow(slab_cache *cache)
{
    // Precondition: the cache's lock must be held
    KDEBUG_ASSERT(spinlock_holding(&cache->lock));

    auto block = buddy_alloc_4k_page();
    if (block == nullptr)
    {
        return block;
    }

    slab *slb = reinterpret_cast<decltype(slb)>(block);
    slb->cache = cache;
    slb->next_free = 0;
    slb->inuse = 0;

    list_add(&slb->slab_link, &cache->free);

    slb->obj_ptr = reinterpret_cast<decltype(slb->obj_ptr)>(((char *)block) + sizeof(slab) + sizeof(slab_bufctl) * cache->obj_count);

    void *obj = slb->obj_ptr;
    for (size_t i = 0; i < cache->obj_count; i++)
    {
        if (cache->ctor)
        {
            cache->ctor(obj, cache, cache->obj_size);
        }

        obj = (void *)((char *)obj + cache->obj_size);
        slb->freelist[i] = i + 1;
    }
    slb->freelist[cache->obj_count - 1] = -1;

    return slb;
}

static inline void slab_destory(slab_cache *cache, slab *slb)
{
    // Precondition: the cache's lock must be held
    KDEBUG_ASSERT(spinlock_holding(&cache->lock));

    void *obj = slb->obj_ptr;
    for (size_t i = 0; i < cache->obj_count; i++)
    {
        if (cache->dtor)
        {
            cache->dtor(obj, cache, cache->obj_size);
        }
    }

    list_remove(&slb->slab_link);
    buddy_free_4k_page(slb);
}

// ATTENTION: BE CAUTIOUS FOR RACE CONDITION
// This function should not be a critial section because there's no modifying
static inline slab *slab_find(slab_cache *cache, void *obj)
{
    list_head *heads[] = {&cache->partial, &cache->full};
    slab *slb = nullptr;

    for (auto head : heads)
    {
        list_head *iter = nullptr;
        list_for(iter, head)
        {
            slab *s = list_entry(iter, slab, slab_link);
            uintptr_t begin = (uintptr_t)s->obj_ptr;
            uintptr_t end = (uintptr_t)(((char *)s->obj_ptr) + cache->obj_size * cache->obj_count);
            if (((uintptr_t)obj) >= begin && ((uintptr_t)obj) < end)
            {
                slb = s;
                break;
            }
        }
    }

    return slb;
}

void allocators::slab_allocator::slab_init(void)
{
    cache_cache.obj_size = sizeof(decltype(cache_cache));
    cache_cache.obj_count = cache_obj_count(cache_cache.obj_size);
    cache_cache.ctor = nullptr;
    cache_cache.dtor = nullptr;

    auto cache_cache_name = "cache_cache";
    strncpy(cache_cache.name, cache_cache_name, CACHE_NAME_MAXLEN);

    spinlock_initlock(&cache_cache.lock, cache_cache_name);
    spinlock_initlock(&cache_head_lock, "cache_head");

    list_init(&cache_cache.full);
    list_init(&cache_cache.partial);
    list_init(&cache_cache.free);

    list_init(&cache_head);
    list_add(&cache_cache.cache_link, &cache_head);

    char sized_cache_name[CACHE_NAME_MAXLEN];
    size_t sized_cache_count = 0;
    for (size_t sz = MIN_SIZED_CACHE_SIZE; sz <= MAX_SIZED_CACHE_SIZE; sz *= 2)
    {

        memset(sized_cache_name, 0, sizeof(sized_cache_name));

        sized_cache_name[0] = 's';
        sized_cache_name[1] = 'i';
        sized_cache_name[2] = 'z';
        sized_cache_name[3] = 'e';
        sized_cache_name[4] = '-';
        sized_cache_name[5] = '-';

        [[maybe_unused]] size_t len = itoa_ex(sized_cache_name + 4, sz, 10);
        sized_caches[sized_cache_count++] = slab_cache_create(sized_cache_name, sz, nullptr, nullptr);
    }

    KDEBUG_ASSERT(sized_cache_count == sized_cache_count);
}

slab_cache *allocators::slab_allocator::slab_cache_create(const char *name, size_t size, ctor_type ctor, dtor_type dtor)
{
    KDEBUG_ASSERT(size < BLOCK_SIZE - sizeof(slab_bufctl));
    slab_cache *ret = reinterpret_cast<decltype(ret)>(slab_cache_alloc(&cache_cache));

    if (ret != nullptr)
    {
        ret->obj_size = size;
        ret->obj_count = cache_obj_count(size);

        ret->ctor = ctor;
        ret->dtor = dtor;

        strncpy(ret->name, name, CACHE_NAME_MAXLEN);
        spinlock_initlock(&ret->lock, ret->name);

        list_init(&ret->full);
        list_init(&ret->partial);
        list_init(&ret->free);

        spinlock_acquire(&cache_head_lock);

        list_add(&ret->cache_link, &cache_head);

        spinlock_release(&cache_head_lock);
    }
    else
    {
        KDEBUG_GENERALPANIC("Insufficient memory for slab initialization.");
    }

    return ret;
}

void *allocators::slab_allocator::slab_cache_alloc(slab_cache *cache)
{
    spinlock_acquire(&cache->lock);

    list_head *entry = nullptr;
    if (!list_empty(&cache->partial))
    {
        entry = cache->partial.next;
    }
    else
    {
        if (list_empty(&cache->free) && slab_cache_grow(cache) == nullptr)
        {
            return nullptr;
        }

        entry = cache->free.next;
    }

    KDEBUG_ASSERT(entry != nullptr);

    list_remove(entry);
    slab *slb = list_entry(entry, slab, slab_link);

    void *ret = (void *)(((uint8_t *)slb->obj_ptr) + slb->next_free * cache->obj_size);

    slb->inuse++;
    slb->next_free = slb->freelist[slb->next_free];

    if (slb->inuse == cache->obj_count)
    {
        list_add(entry, &cache->full);
    }
    else
    {
        list_add(entry, &cache->partial);
    }

    spinlock_release(&cache->lock);

    return ret;
}

void allocators::slab_allocator::slab_cache_destroy(slab_cache *cache)
{
    spinlock_acquire(&cache->lock);
    list_head *heads[] = {&cache->full, &cache->partial, &cache->free};
    for (auto head : heads)
    {
        auto entry = head->next;
        while (head != entry)
        {
            auto slb = list_entry(entry, slab, slab_link);
            entry = entry->next;
            slab_destory(cache, slb);
        }
    }
    spinlock_release(&cache->lock);

    slab_cache_free(&cache_cache, cache);
}

void allocators::slab_allocator::slab_cache_free(slab_cache *cache, void *obj)
{
    KDEBUG_ASSERT(obj != nullptr && cache != nullptr);

    slab *slb = slab_find(cache, obj);
    KDEBUG_ASSERT(slb != nullptr);

    size_t offset = (((uintptr_t)obj) - ((uintptr_t)slb->obj_ptr)) / cache->obj_size;

    spinlock_acquire(&cache->lock);
    list_remove(&slb->slab_link);
    slb->freelist[offset] = slb->next_free;
    slb->next_free = offset;
    slb->inuse--;

    if (slb->inuse == 0)
    {
        list_add(&slb->slab_link, &cache->free);
    }
    else
    {
        list_add(&slb->slab_link, &cache->partial);
    }

    spinlock_release(&cache->lock);
}

size_t allocators::slab_allocator::slab_cache_shrink(slab_cache *cache)
{
    size_t count = 0;
    auto entry = cache->free.next;
    auto head = &cache->free;
    spinlock_acquire(&cache->lock);

    while (head != entry)
    {
        auto slb = list_entry(entry, slab, slab_link);
        entry = entry->next;
        slab_destory(cache, slb);
        count++;
    }

    spinlock_release(&cache->lock);

    return count;
}

size_t allocators::slab_allocator::slab_cache_reap()
{
    size_t count = 0;
    list_head *iter = nullptr;
    list_for(iter, &cache_head)
    {
        count += slab_cache_shrink(list_entry(iter, slab_cache, cache_link));
    }
    return count;
}
