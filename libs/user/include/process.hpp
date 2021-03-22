#pragma once

#include "kernel_object.hpp"

#include "compiler/compiler_extensions.hpp"

#include "handle_type.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API error_code get_current_process(OUT object::handle_type* out);

DIONYSUS_API error_code get_process_by_id(OUT object::handle_type* out, object::koid_type id);
DIONYSUS_API error_code get_process_by_name(OUT object::handle_type* out, const char* name);