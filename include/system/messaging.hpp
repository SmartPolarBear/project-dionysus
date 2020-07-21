#pragma once

#include "system/types.h"

constexpr size_t MESSAGE_MAGIC = 0x3caffe3effac;

struct MessageHeader
{
	const size_t magic = MESSAGE_MAGIC;
	size_t type;
}__attribute__((packed));

struct BasicMessage
{
	const MessageHeader header;
};

enum MessageType
{
	REGISTER_SERVICE_MSG = 1,
	ADD_MSG,
	ADD_RET_MSG,
};

struct RegisterServiceMessage
{
	const MessageHeader header{ .type=REGISTER_SERVICE_MSG };
	size_t service_id;
};

struct AddMessage
{
	const MessageHeader header{ .type=ADD_MSG };
	size_t a, b;
};

struct AddRetMessage
{
	const MessageHeader header{ .type=ADD_RET_MSG };
	size_t ret;
};