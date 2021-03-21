#pragma once

#include "system/types.h"
#include "system/error.hpp"
#include "system/time.hpp"

#include "messages.hpp"
#include "handle_type.hpp"

#include "dionysus_api.hpp"

DIONYSUS_API error_code ipc_load_message(task::ipc::message* msg);

DIONYSUS_API error_code ipc_send(object::handle_type target, time_type timeout);

DIONYSUS_API error_code ipc_receive(object::handle_type from, time_type timeout);
