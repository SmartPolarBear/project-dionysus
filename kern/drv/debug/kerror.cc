
#include "arch/amd64/x86.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/debug/kerror.h"

#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/types.h"

#include <cstring>

#pragma clang diagnostic push

// we must use array designators here whatsoever. And clang supports this.
#pragma clang diagnostic ignored "-Wc99-designator"

const char *err_msg[] = {[ERROR_UNKOWN] = "Unkown error in kernel.",
                         [ERROR_INVALID_DATA] = "Invalid data value.",
                         [ERROR_MEMORY_ALLOC] = "Can't allocate enough memory.",
                         [ERROR_REWRITE] = "Rewrite unrewritable data",
                         [ERROR_VMA_NOT_FOUND] = "can't find a VMA",
                         [ERROR_PAGE_NOT_PERSENT] = "Page not persent"};

const char *err_title[] = {[ERROR_UNKOWN] = "ERROR_UNKOWN",
                           [ERROR_INVALID_DATA] = "ERROR_INVALID_DATA",
                           [ERROR_MEMORY_ALLOC] = "ERROR_MEMORY_ALLOC",
                           [ERROR_REWRITE] = "ERROR_REWRITE",
                           [ERROR_VMA_NOT_FOUND] = "ERROR_VMA_NOT_FOUND",
                           [ERROR_PAGE_NOT_PERSENT] = "ERROR_PAGE_NOT_PERSENT"};

const char *kdebug::error_message(error_code code)
{
    return err_msg[static_cast<size_t>(code)];
}

const char *kdebug::error_title(error_code code)
{
    return err_title[static_cast<size_t>(code)];
}

#pragma clang diagnostic pop
