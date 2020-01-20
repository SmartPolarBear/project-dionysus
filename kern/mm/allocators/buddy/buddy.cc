#include "sys/buddy_alloc.h"

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "./buddy.h"

using namespace buddy_internal::types;
using namespace buddy_internal::constants;

buddy_struct buddy_internal::buddy;

using buddy_internal::buddy;

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
            auto mark = buddy_internal::order_get_mark(i + MIN_ORD, j);
            mark->links = buddy_internal::links(MARK_EMPTY, MARK_EMPTY);
            mark->bitmap = 0;
        }

        total_offset += n_marks;
        n_marks <<= 1;
    }

    buddy.start_mem = buddy_internal::align_up(buddy.start + total_offset * sizeof(mark), 1 << ORD_COUNT);

    for (uintptr_t i = buddy.start_mem; i < buddy.end; i += (1 << ORD_COUNT))
    {
        buddy_internal::buddy_free((void *)i, ORD_COUNT);
    }

    buddy.initialized = true;
}

void *vm::buddy_alloc(size_t sz)
{
}

void vm::buddy_free(void *m)
{
}