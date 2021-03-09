#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "messages.hpp"

extern "C" error_code ipc_load_message(task::ipc::message* msg);
