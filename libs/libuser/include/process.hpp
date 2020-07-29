//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.h"
#include "system/messaging.hpp"

extern "C" error_code terminate(error_code e);
extern "C" error_code set_heap(uintptr_t* size);

void heap_free(void* ap);
void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags);

extern "C" error_code ipc_send(process_id pid, IN const void* msg, size_t size);
extern "C" error_code ipc_receive(OUT void* msg);

template<typename TMsg>
static inline void initialize_message(TMsg& msg, MessageType type, process_id pid)
{
	msg.header.to = pid;
	msg.header.size = sizeof(TMsg);
	msg.header.type = type;
}