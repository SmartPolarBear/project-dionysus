#if !defined(__INCLUDE_SYS_ALLOCATORS_SLAB_ALLOC_H)
#define __INCLUDE_SYS_ALLOCATORS_SLAB_ALLOC_H

#include "sys/types.h"

#include "lib/libkern/data/list.h"

#include "drivers/debug/hresult.h"
#include "drivers/lock/spinlock.h"

namespace allocators
{

namespace slab_allocator
{
struct slab_cache;

using ctor_type = void (*)(void *, slab_cache *, size_t);
using dtor_type = ctor_type;
using slab_bufctl = size_t;

constexpr size_t CACHE_NAME_MAXLEN = 32;
constexpr size_t BLOCK_SIZE = 4096;

constexpr size_t MAX_SIZED_CACHE_SIZE = 2048;
constexpr size_t MIN_SIZED_CACHE_SIZE = 16;
constexpr size_t SIZED_CACHE_COUNT = log2p1(MAX_SIZED_CACHE_SIZE / MIN_SIZED_CACHE_SIZE);

struct slab_cache
{
    list_head full, partial, free;
    size_t obj_size, obj_count;
    ctor_type ctor;
    dtor_type dtor;
    char name[CACHE_NAME_MAXLEN];
    list_head cache_link;

    lock::spinlock lock;
};

struct slab
{
    size_t ref;
    slab_cache *cache;
    size_t inuse, next_free;
    void *obj_ptr;
    slab_bufctl *freelist;
    list_head slab_link;
};

void slab_init(void);
slab_cache *slab_cache_create(const char *name, size_t size, ctor_type ctor, dtor_type dtor);
void *slab_cache_alloc(slab_cache *cache);
void slab_cache_destroy(slab_cache *cache);
void slab_cache_free(slab_cache *cache, void *obj);
size_t slab_cache_shrink(slab_cache *cache);
size_t slab_cache_reap();

} // namespace slab_allocator

} // namespace allocators

#endif // __INCLUDE_SYS_ALLOCATORS_SLAB_ALLOC_H
