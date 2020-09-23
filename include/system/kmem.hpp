#pragma once

#include "system/error.hpp"
#include "system/types.h"

#include "data/List.h"

#include "drivers/lock/spinlock.h"

namespace memory
{

namespace kmem
{
struct kmem_cache;

using kmem_ctor_type = void (*)(void *, kmem_cache *, size_t);
using kmem_dtor_type = kmem_ctor_type;
using kmem_bufctl = size_t;

constexpr size_t KMEM_CACHE_NAME_MAXLEN = 32;

constexpr size_t KMEM_MAX_SIZED_CACHE_SIZE = 10240;
constexpr size_t KMEM_MIN_SIZED_CACHE_SIZE = 16;
constexpr size_t KMEM_SIZED_CACHE_COUNT = log2p1(KMEM_MAX_SIZED_CACHE_SIZE / KMEM_MIN_SIZED_CACHE_SIZE);

enum kmem_cache_flags
{
    KMEM_CACHE_4KALIGN = 0b1,
};

struct kmem_cache
{
    list_head full, partial, free;
    size_t obj_size, obj_count;
    size_t flags;
    kmem_ctor_type ctor;
    kmem_dtor_type dtor;
    char name[KMEM_CACHE_NAME_MAXLEN];
    list_head cache_link;

    lock::spinlock lock;
};

void kmem_init(void);
kmem_cache *kmem_cache_create(const char *name,
                              size_t size,
                              kmem_ctor_type ctor = nullptr,
                              kmem_dtor_type dtor = nullptr,
                              size_t flags = 0);

void *kmem_cache_alloc(kmem_cache *cache);
void kmem_cache_destroy(kmem_cache *cache);
void kmem_cache_free(kmem_cache *cache, void *obj);
size_t kmem_cache_shrink(kmem_cache *cache);
size_t kmem_cache_reap();

} // namespace kmem

} // namespace memory

