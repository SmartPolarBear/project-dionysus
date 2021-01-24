#pragma once

#include "system/types.h"

namespace kdebug
{
void kdebug_print_backtrace();
void kdebug_get_caller_pcs(size_t buflen, uintptr_t* pcs);

}