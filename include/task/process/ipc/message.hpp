#pragma once

namespace task::ipc
{

class endpoint;

struct message_tag
{

};

class message_item
{
 public:
	virtual void on_sending(endpoint* ep) = 0;
};

class short

}