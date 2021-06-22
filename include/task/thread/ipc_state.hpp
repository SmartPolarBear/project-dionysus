// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by bear on 6/20/21.
//

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
#include "kbl/lock/semaphore.hpp"

#include "system/cls.hpp"
#include "system/time.hpp"
#include "system/deadline.hpp"

#include "syscall_handles.hpp"

#include "drivers/apic/traps.h"

#include "task/thread/wait_queue.hpp"
#include "task/thread/cpu_affinity.hpp"
#include "task/thread/user_stack.hpp"

#include "task/scheduler/scheduler_config.hpp"
#include "task/ipc/message.hpp"

#include "memory/address_space.hpp"

#include <compare>

namespace task
{

class ipc_state
{
 public:
	static constexpr size_t MR_SIZE = 64;
	static constexpr size_t BR_SIZE = 33;

	ipc_state() = delete;

	explicit ipc_state(thread* parent) : parent_(parent)
	{
	}

	ipc_state(const ipc_state&) = delete;
	ipc_state& operator=(const ipc_state&) = delete;

	error_code receive([[maybe_unused]]thread* from, const deadline& ddl) TA_REQ(!global_thread_lock);

	error_code send(thread* to, const deadline& ddl) TA_REQ(!global_thread_lock);

	void load_message(ipc::message* msg)TA_REQ(!global_thread_lock);

	void store_message(ipc::message* msg) TA_REQ(!global_thread_lock);

	void copy_mrs(thread* another, size_t st, size_t cnt);

	template<typename T>
	T get_typed_item(size_t index)
	{
		return *reinterpret_cast<T*>(mr_ + index);
	}

	ipc::message_register_type get_mr(size_t index)
	{
		return mr_[index];
	}

	void set_mr(size_t index, ipc::message_register_type value)
	{
		mr_[index] = value;
	}

	ipc::message_register_type get_br(size_t index)
	{
		return br_[index];
	}

	void set_br(size_t index, ipc::message_register_type value)
	{
		br_[index] = value;
	}

	[[nodiscard]] ipc::message_tag get_message_tag();

	[[nodiscard]] ipc::message_acceptor get_acceptor();

	/// \brief set the message tag to mrs. will reset mr_count_, which influence exist items
	/// \param tag
	void set_message_tag(const ipc::message_tag* tag) noexcept;

	/// \brief set acceptor to brs. will reset mr_count_, which influence exist items
	/// \param acc
	void set_acceptor(const ipc::message_acceptor* acc) noexcept;

 private:
	void load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs) TA_REQ(lock_);

	void store_mrs_locked(size_t st, ktl::span<ipc::message_register_type> mrs) TA_REQ(lock_);

	void copy_mrs_to_locked(thread* another, size_t st, size_t cnt) TA_REQ(lock_);

	/// \brief set the message tag to mrs. will reset mr_count_, which influence exist items
	/// \param tag
	void set_message_tag_locked(const ipc::message_tag* tag) noexcept  TA_REQ(lock_);

	/// \brief handle extended items like strings and map/grant items
	/// \param to which thread to send extended items
	/// \return
	error_code send_extended_items(thread* to);

	/// \brief message registers
	ipc::message_register_type mr_[MR_SIZE]{ 0 };

	size_t mr_count_{ 0 };

	/// \brief buffer registers
	ipc::buffer_register_type br_[BR_SIZE]{ 0 };

	size_t br_count_{ 0 };

	[[maybe_unused]]thread* parent_{ nullptr };

	kbl::semaphore f_{ 0 }; // indicate that if items has been written but not yet read

	kbl::semaphore e_{ 1 }; // indicate that if there's room to write

	mutable lock::spinlock lock_{ "ipc_state" };
};
}