#include "sys/buddy_alloc.h"

using namespace vm;

constexpr size_t MAX_ORD = 12;
constexpr size_t MIN_ORD = 6;

constexpr size_t ORD_COUNT = MAX_ORD - MIN_ORD + 1;
constexpr uint16_t MARK_EMPTY = 0xFFFF;

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

void vm::buddy_init(void *as, void *ae)
{
}

void *vm::buddy_alloc(size_t sz)
{
}

void vm::buddy_free(void *m)
{
}