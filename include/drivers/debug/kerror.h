#if !defined(__INCLUDE_DRIVERS_DEBUG_KERROR_H)
#define __INCLUDE_DRIVERS_DEBUG_KERROR_H

#include "sys/error.h"

namespace kdebug
{
const char *error_message(error_code code);
const char *error_title(error_code code);
} // namespace kdebug

#endif // __INCLUDE_DRIVERS_DEBUG_KERROR_H
