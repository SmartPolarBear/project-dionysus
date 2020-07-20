//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/types.h"
#include "system/error.h"
#include "system/messaging.hpp"

extern "C" error_code app_terminate(error_code e);

extern "C" error_code ipc_send(IN const MessageBase* msg);
extern "C" error_code ipc_receive(OUT  MessageBase* msg);

template<typename TMsg>
static inline void initialize_message(TMsg& msg, MessageType type, process_id pid)
{
	msg.header.to = pid;
	msg.header.size = sizeof(TMsg);
	msg.header.type = type;
}