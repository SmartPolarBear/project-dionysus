//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.h"

extern "C" error_code app_terminate(error_code e);
extern "C" error_code send(size_t pid, size_t msg_sz, void* msg);
extern "C" error_code receive(void** msg, size_t* sz);