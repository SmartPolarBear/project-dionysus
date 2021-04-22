#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/data/list.hpp"
#include "kbl/data/name.hpp"
#include "kbl/checker/canary.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/concepts.hpp"
#include "kbl/lock/lock_guard.hpp"

#include "system/cls.hpp"
#include "system/time.hpp"
#include "system/deadline.hpp"

#include "drivers/apic/traps.h"

#include <compare>

namespace task
{
extern lock::spinlock global_thread_lock;

class thread;

struct wait_queue_list_node_trait
{
	using reference_type = thread&;
	using pointer_type = thread*;
	using reference_return_type = kbl::list_link<thread, lock::spinlock>&;
	using pointer_return_type = kbl::list_link<thread, lock::spinlock>*;

	static reference_return_type node_link(reference_type element);
	static reference_return_type node_link(pointer_type NONNULL element);

	static pointer_return_type NONNULL node_link_ptr(reference_type element)
	{
		return &node_link(element);
	}

	static pointer_return_type NONNULL node_link_ptr(pointer_type NONNULL element)
	{
		return &node_link(element);
	}
};

static_assert(kbl::NodeTrait<wait_queue_list_node_trait, thread, lock::spinlock>);

class wait_queue
{
 public:
	using wait_queue_list_type = kbl::intrusive_list<thread, lock::spinlock, wait_queue_list_node_trait, true>;

	enum class [[clang::enum_extensibility(closed)]] interruptible : bool
	{
		No, Yes
	};

	enum class [[clang::enum_extensibility(closed)]] resource_ownership
	{
		Normal,
		Reader
	};

	wait_queue() = default;

	~wait_queue();

	wait_queue(wait_queue&) = delete;
	wait_queue(wait_queue&&) = delete;

	wait_queue& operator=(wait_queue&) = delete;
	wait_queue& operator=(wait_queue&&) = delete;

	static error_code unblock_thread(thread* t, error_code code) TA_REQ(global_thread_lock);

	error_code block(interruptible intr) TA_REQ(global_thread_lock);

	error_code block(interruptible intr, const deadline& deadline)TA_REQ(global_thread_lock);

	error_code block_etc(const deadline& deadline,
		uint32_t signal_mask,
		resource_ownership reason,
		interruptible intr) TA_REQ(global_thread_lock);

	thread* peek() TA_REQ(global_thread_lock);

	bool wake_one(bool reschedule, error_code code) TA_REQ(global_thread_lock);
	void wake_all(bool reschedule, error_code code) TA_REQ(global_thread_lock);

	bool empty() const TA_REQ(global_thread_lock);

	size_t size() const TA_REQ(global_thread_lock)
	{
		return block_list_.size();
	}
 private:
	static void timeout_handle(struct scheduler_timer*, time_type now, void* arg) TA_REL(global_thread_lock);

	void dequeue(thread* t, error_code err) TA_REQ(global_thread_lock);

	wait_queue_list_type block_list_;
};


}