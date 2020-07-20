#pragma once

#include "system/types.h"

struct MessageHeader
{
	// fill by sender
	size_t type;
	// fill by sender
	size_t size;

	// fill by sender
	process_id to;

	// fill by process_ipc_send
	process_id from;

	uint8_t data[0];
}__attribute__((packed));

struct MessageBase
{
	MessageHeader header;
};

enum MessageType
{
	kMsgTypeRegisterServiceMessage,
	kMsgTypeAddMessage,
	kMsgTypeAddRetMessage,
};

struct RegisterServiceMessage : MessageBase
{
	size_t service_id;
};

struct AddMessage : MessageBase
{
	size_t a, b;
};

struct AddRetMessage : MessageBase
{
	size_t ret;
};