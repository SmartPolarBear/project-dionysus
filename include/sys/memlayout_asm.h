#pragma once

#if !defined(__ASSEMBLER__)
#error "This header is only for assemblies"
#endif

#define PMM_PAGE_SIZE 4096

#define USER_TOP 0x00007fffffffffff
#define USER_STACK_TOP (USER_TOP - 2 * PMM_PAGE_SIZE)