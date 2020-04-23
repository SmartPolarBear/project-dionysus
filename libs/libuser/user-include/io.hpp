#pragma once

#include "sys/types.h"
#include "sys/error.h"

extern "C" error_code KernelWriteHello(const char *extra);
