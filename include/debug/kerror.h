#pragma once

#include "system/error.hpp"

namespace kdebug
{
const char *error_message(error_code code);
const char *error_title(error_code code);
} // namespace kdebug


