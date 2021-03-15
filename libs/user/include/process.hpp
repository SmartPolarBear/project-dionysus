#pragma once

#include "compiler/compiler_extensions.hpp"

#include "handle_type.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API error_code get_current_process(OUT object::handle_type *out);
