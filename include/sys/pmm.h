#if !defined(__INCLUDE_SYSY_PMM_H)
#define __INCLUDE_SYSY_PMM_H

#include "sys/memlayout.h"
#include "sys/types.h"

namespace pmm
{
constexpr size_t PMM_MANAGER_NAME_MAXLEN = 32;
struct pmm_manager_info
{
    char name[PMM_MANAGER_NAME_MAXLEN];

    void (*init)(void);                             // initialize internal description&management data structure
                                                    // (free block list, number of free block) of XXX_pmm_manager
    void (*init_memmap)(page_info *base, size_t n); // setup description&management data structcure according to
                                                    // the initial free physical memory space
    page_info *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm
    void (*free_pages)(page_info *base, size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
    size_t (*get_free_pages_count)(void);           // return the number of free pages
};

extern pmm_manager_info *pmm_manager;

extern page_info *pages;
extern size_t page_count;

void init_pmm(void);

page_info *alloc_pages(size_t n);
void free_pages(page_info *base, size_t n);
size_t get_free_page_count(void);

static inline page_info *alloc_page(void)
{
    return alloc_pages(1);
}

static inline void free_page(page_info *pg)
{
    free_pages(pg, 1);
}

static inline size_t page_to_physical_page_number(page_info *pg)
{
    return pg - pages;
}

} // namespace pmm

#endif // __INCLUDE_SYSY_PMM_H
