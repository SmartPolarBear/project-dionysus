#pragma once

#include "system/types.h"

namespace kdebug
{
void kdebug_print_backtrace();
size_t kdebug_get_backtrace(uintptr_t* pcs);

}