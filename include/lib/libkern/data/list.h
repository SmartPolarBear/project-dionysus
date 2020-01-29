#if !defined(__INCLUDE_LIB_LIBKERN_DATA_LIST_H)
#define __INCLUDE_LIB_LIBKERN_DATA_LIST_H

#include "sys/types.h"

namespace libk
{

namespace __list_internals
{

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

static inline void __list_remove_entry(list_head *entry)
{
    __list_remove(entry->prev, entry->next);
}

} // namespace __list_internals

// initialize the list
static inline void list_init(list_head *head)
{
    __list_internals::__list_init(head);
}

// add the new node after the specified head
static inline void list_add(list_head *newnode, list_head *head)
{
    __list_internals::__list_add(newnode, head, head->next);
}

// add the new node before the specified head
static inline void list_add_tail(list_head *newnode, list_head *head)
{
    __list_internals::__list_add(newnode, head->prev, head);
}

// delete the entry
static inline void list_remove(list_head *entry)
{
    __list_internals::__list_remove_entry(entry);
    entry->prev = nullptr;
    entry->next = nullptr;
}

// replace the old entry with newnode
static inline void list_replace(list_head *old, list_head *newnode)
{
    newnode->next = old->next;
    newnode->next->prev = newnode;
    newnode->prev = old->prev;
    newnode->prev->next = newnode;
}

static inline bool list_empty(const list_head *head)
{
    return (head->next) == head;
}
// swap e1 and e2
static inline void list_swap(list_head *e1, list_head *e2)
{
    auto *pos = e2->prev;
    list_remove(e2);
    list_replace(e1, e2);

    if (pos == e1)
    {
        pos = e2;
    }

    list_add(e1, pos);
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for(pos, head) \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

static inline void list_for_each(list_head *head, list_foreach_func func)
{
    for (auto pos = head->next; pos != head; pos = pos->next)
    {
        func(pos);
    }
}

} // namespace libk

#endif // __INCLUDE_LIB_LIBKERN_DATA_LIST_H
