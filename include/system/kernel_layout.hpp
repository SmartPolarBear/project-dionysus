/// \brief  this file contains declarations of symbols in kernel.ld
#pragma once

#include "system/types.h"

extern "C" void* text_start;
extern "C" void* text_end;

extern "C" void* bss_start;
extern "C" void* bss_end;

extern "C" void* data_start;
extern "C" void* data_end;

using ctor_type = void (*)();
using dtor_type = void (*)();

extern "C" ctor_type start_ctors;
extern "C" ctor_type end_ctors;

extern "C" dtor_type start_dtors;
extern "C" dtor_type end_dtors;



