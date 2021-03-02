#pragma once
#include "object/kernel_object.hpp"

#include "kbl/data/name.hpp"
#include "kbl/lock/spinlock.h"

#include "task/thread/wait_queue.hpp"
#include "task/process/ipc/message.hpp"


namespace task::ipc
{



/// \brief an endpoint is a set of process waiting to receive
class endpoint final
	: object::kernel_object<endpoint>
{
 public:

	/// \brief send a message
	/// \param tag the message tag
	/// \return ERROR_IPC_NO_RECEIVER if not a single process is waiting to receive
	error_code try_send(message_tag* tag);

 private:
	wait_queue receivers_{};

	mutable kbl::name<32> name_;
	mutable lock::spinlock lock_;
};

}