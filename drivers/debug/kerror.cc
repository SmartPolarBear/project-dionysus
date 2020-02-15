
#include "arch/amd64/x86.h"

#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "sys/memlayout.h"
#include "sys/types.h"

#include "lib/libc/string.h"

using kdebug::error;

void kdebug::fill_error(hresult code, const char *desc, OUT error &err)
{
    err.code = code;

    size_t desc_len = strlen(desc);
    if (desc_len > error::DESC_LEN)
    {
        strncpy(err.desc, desc, error::DESC_LEN);
        err.desc[error::DESC_LEN] = '\0';
    }
    else
    {
        strncpy(err.desc, desc, desc_len);
        err.desc[desc_len] = '\0';
    }
}