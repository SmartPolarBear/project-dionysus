#pragma once

#include "system/types.h"

#include "internals/elf.hpp"

#include "task/process/process.hpp"


error_code load_binary(IN task::process* proc,
	IN uint8_t* bin,
	IN size_t bin_sz,
	OUT uintptr_t* entry_addr);