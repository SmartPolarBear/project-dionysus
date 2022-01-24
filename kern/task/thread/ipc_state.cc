#include "../include/syscall.h"
#include "internals/thread.hpp"

#include "task/thread/thread.hpp"
#include "task/process/process.hpp"
#include "task/scheduler/scheduler.hpp"

#include "system/mmu.h"
#include "system/vmm.h"
#include "system/kmalloc.hpp"
#include "system/scheduler.h"
#include "system/deadline.hpp"

#include "drivers/acpi/cpu.h"

#include "kbl/lock/lock_guard.hpp"

#include <gsl/util>

#include <utility>
#include <task/thread/ipc_state.hpp>

using namespace task;
using namespace lock;

using namespace ipc;

void task::ipc_state::copy_mrs_to_locked(thread* another, size_t st, size_t cnt)
{
	memmove(&another->ipc_state_.mr_[st], &mr_[st], sizeof(message_register_type) * cnt);
}

void task::ipc_state::load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs)
{
	memmove(&mr_[start], mrs.data(), sizeof(message_register_type) * mrs.size());
}

error_code ipc_state::copy_string_locked(thread* from_t, uintptr_t from, thread* to_t, uintptr_t to, size_t len) TA_REQ(
	lock_)
{
	lock_guard g{ to_t->ipc_state_.lock_ };

	if (!VALID_USER_PTR(from))
	{
		return -ERROR_INVALID_ACCESS;
	}

	if (!VALID_USER_PTR(to))
	{
		return -ERROR_INVALID_ACCESS;
	}

	if (!VALID_USER_PTR(from + len - sizeof(uint8_t)))
	{
		return -ERROR_INVALID_ACCESS;
	}

	if (!VALID_USER_PTR(to + len - sizeof(uint8_t)))
	{
		return -ERROR_INVALID_ACCESS;
	}

	memmove((void*)to, (void*)from, len);

	return ERROR_SUCCESS;
}

void task::ipc_state::store_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs)
{
	auto mr = mr_ + start;
	for (auto& m: mrs)
	{
		m = *mr++;
	}
}

[[nodiscard]] ipc::message_tag task::ipc_state::get_message_tag()
{
	return static_cast<ipc::message_tag>(mr_[0]);
}

[[nodiscard]] ipc::message_acceptor task::ipc_state::get_acceptor()
{
	return static_cast<ipc::message_acceptor>(br_[0]);
}

void task::ipc_state::set_message_tag_locked(const ipc::message_tag* tag) noexcept
{
	mr_[0] = tag->raw();
	mr_count_ = 1;
}

void task::ipc_state::set_acceptor(const ipc::message_acceptor* acc) noexcept
{
	br_[0] = acc->raw();
	br_count_ = 1;
}

error_code task::ipc_state::send_extended_items(thread* to)
{
	auto acceptor = to->ipc_state_.get_acceptor();

	auto from = cur_thread.get();
	auto tag = from->ipc_state_.get_message_tag();

	uint64_t br_index = 1;

	for (size_t idx = tag.untyped_count() + 1; idx < tag.typed_count();)
	{
		auto mr = from->ipc_state_.get_mr(idx);

		if (static_cast<ipc::message_item_types>(mr & 0xF) == ipc::message_item_types::MAP)
		{
			auto map = from->ipc_state_.get_typed_item<ipc::map_item>(idx);
			auto[send, receive] = acceptor.get_send_receive_region(map.page(), map.base());

			{
				lock::lock_guard g{ lock_ };
				copy_mrs_to_locked(to, idx++, 1);
				copy_mrs_to_locked(to, idx++, 1);
			}

			auto ret = from->address_space()->fpage_grant(to->address_space(), send, receive);
			if (has_error(ret))
			{
				return get_error_code(ret);
			}
		}
		else if (static_cast<ipc::message_item_types>(mr & 0xF) == ipc::message_item_types::GRANT)
		{
			auto grant = from->ipc_state_.get_typed_item<ipc::grant_item>(idx);
			auto[send, receive] = acceptor.get_send_receive_region(grant.page(), grant.base());

			{
				lock::lock_guard g{ lock_ };
				copy_mrs_to_locked(to, idx++, 1);
				copy_mrs_to_locked(to, idx++, 1);
			}

			auto ret = from->address_space()->fpage_grant(to->address_space(), send, receive);
			if (has_error(ret))
			{
				return get_error_code(ret);

			}
		}
		else if (static_cast<ipc::message_item_types>(mr & 0xF) == ipc::message_item_types::STRING)
		{
			if (!acceptor.allow_string())
			{
				return -ERROR_INVALID;
			}

			auto src_item = from->ipc_state_.get_typed_item<ipc::string_item>(idx);

			{
				lock_guard g{ lock_ };

				auto dst_br = get_br(br_index);
				br_index += 2;

				if (static_cast<ipc::message_item_types>(get_br(br_index + 1) & 0xF) != ipc::message_item_types::STRING)
				{
					return -ERROR_INVALID;
				}

				if (static_cast<size_t>(get_br(br_index + 1) >> 10ull) < src_item.length())
				{
					return -ERROR_INVALID;
				}

				if (auto err = copy_string_locked(from, src_item.address(), to, (uintptr_t)dst_br, src_item.length());
					err != ERROR_SUCCESS)
				{
					return err;
				}

				decltype(get_br(br_index + 1)) new_br = src_item.length() << 10ull;
				new_br |= (get_br(br_index + 1) & 0x3FF);

				set_br(br_index + 1, new_br);
			}
		}
		else
		{
			return -ERROR_INVALID;
		}
	}

	return ERROR_SUCCESS;
}

error_code task::ipc_state::send(thread* to, const deadline& ddl)
{
	if (auto err = to->get_ipc_state()->e_.wait(ddl);err != ERROR_SUCCESS)
	{
		KDEBUG_GERNERALPANIC_CODE(err);
	}

	{
		lock::lock_guard g{ lock_ };

		to->ipc_state_.sender_ = parent_;

		copy_mrs_to_locked(to, 0, task::ipc_state::MR_SIZE);

		if (auto err = send_extended_items(to);err != ERROR_SUCCESS)
		{
			return err;
		}

		to->get_ipc_state()->f_.signal();
	}

	return ERROR_SUCCESS;
}

error_code task::ipc_state::receive(thread* from, const deadline& ddl)
{
	if (auto err = f_.wait(ddl);err != ERROR_SUCCESS)
	{
		KDEBUG_GERNERALPANIC_CODE(err);
	}

	{
		lock_guard g{ lock_ };

		KDEBUG_ASSERT_MSG(this->get_message_tag().typed_count() != 0 || this->get_message_tag().untyped_count() != 0,
			"Empty message isn't valid");

		if (sender_ != from)
		{
			return -ERROR_IPC_NOT_THE_SENDER;
		}
	}

	// do not call 	e_.signal(); to avoid multiple sender overwrite the buffer

	return ERROR_SUCCESS;
}

void task::ipc_state::load_message(ipc::message* msg)
{
	auto tag = msg->get_tag();

	lock::lock_guard g{ lock_ };

	set_message_tag_locked(&tag);

	load_mrs_locked(1, msg->get_items_span());
}

void task::ipc_state::store_message(message* msg)
{
	{
		lock_guard g{ lock_ };

		msg->set_tag(get_message_tag());

		store_mrs_locked(1, msg->get_items_span(get_message_tag()));
	}

	e_.signal(); // allow next sender to send
}

error_code ipc_state::wait(const deadline& ddl) TA_REQ(!global_thread_lock)
{
	if (auto err = f_.wait(ddl);err != ERROR_SUCCESS)
	{
		KDEBUG_GERNERALPANIC_CODE(err);
	}

	{
		lock_guard g{ lock_ };

		KDEBUG_ASSERT_MSG(this->get_message_tag().typed_count() != 0 || this->get_message_tag().untyped_count() != 0,
			"Empty message isn't valid");
	}

	// do not call 	e_.signal(); to avoid multiple sender overwrite the buffer

	return ERROR_SUCCESS;
}


