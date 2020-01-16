#if !defined(__INCLUDE_LIB_LIBKERN_DATA_LIST_H)
#define __INCLUDE_LIB_LIBKERN_DATA_LIST_H

#include "sys/types.h"

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr)-offsetof(type, member)))

static inline void __list_init(list_head *head)
{
    head->next = head;
    head->prev = head;
}

static inline void __list_add(list_head *newnode, list_head *prev, list_head *next)
{
    prev->next = newnode;
    next->prev = newnode;

    newnode->next = next;
    newnode->prev = prev;
}

static inline void __list_remove(list_head *prev, list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

namespace libk
{

static inline void list_init(list_head *head)
{
    __list_init(head);
}

static inline void list_add(list_head *newnode, list_head *head)
{
    __list_add(newnode, head, head->next);
}

static inline void list_remove(list_head *entry)
{
    __list_remove(entry->prev, entry->next);
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

static inline void list_for_each(list_head *head, list_foreach_func func)
{
    for (auto pos = head->next; pos != head; pos = pos->next)
    {
        func(pos);
    }
}

} // namespace klib

#endif // __INCLUDE_LIB_LIBKERN_DATA_LIST_H
