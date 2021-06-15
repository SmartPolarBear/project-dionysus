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

using namespace task;

using lock::lock_guard;

#ifdef  IPC_MRCOPY_USE_SIMD
#error "IPC_MRCOPY_USE_SIMD can't be defined"
#endif

#define IPC_MRCOPY_USE_SIMD

using namespace ipc;

void task::ipc_state::copy_mrs_to_locked(thread* another, size_t st, size_t cnt)
{

#ifdef IPC_MRCOPY_USE_SIMD
	register_t dummy = 0xdeadbeaf;
	asm volatile (
	"repnz movsq (%%rsi), (%%rdi)\n"
	: /* output */
	"=S"(dummy), "=D"(dummy), "=c"(dummy)
	: /* input */
	"c"(cnt), "S"(&mr_[st]),
	"D"(&another->ipc_state_.mr_[st]));
#elif
	memmove(&another->ipc_state_.mr_[st], &mr_[st], sizeof(decltype(mr_[0])) * cnt);
#endif
}

void task::ipc_state::load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs) TA_REQ(parent_->lock)
{
#ifdef IPC_MRCOPY_USE_SIMD
	register_t dummy = 0xdeadbeaf;
	asm volatile (
	"repnz movsq (%%rsi), (%%rdi)\n"
	: /* output */
	"=S"(dummy), "=D"(dummy), "=c"(dummy)
	: /* input */
	"c"(mrs.size()), "S"(mrs.data()),
	"D"(&mr_[start]));
#elif
	memmove(mrs.data(), &mr_[start], sizeof(decltype(mr_[0])) * mrs.size());
#endif

}

void task::ipc_state::store_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs) TA_REQ(parent_->lock)
{
	auto mr = mr_ + start;
	for (auto& m:mrs)
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

void task::ipc_state::set_message_tag_locked(const ipc::message_tag* tag) noexcept TA_REQ(parent_->lock)
{
	mr_[0] = tag->raw();
	mr_count_ = 1;
}

void task::ipc_state::set_acceptor(const ipc::message_acceptor* acc) noexcept
{
	br_[0] = acc->raw();
	br_count_ = 1;
}

error_code task::ipc_state::receive_locked(thread* from, const deadline& ddl)
{
	this->state_ = IPC_RECEIVING;

	error_code err = ERROR_SUCCESS;
	if (from->ipc_state_.state_ != IPC_SENDING)
	{
		err = from->ipc_state_.receiver_wait_queue_.block(wait_queue::interruptible::No, ddl);
	}

	sender_wait_queue_.wake_all(false, ERROR_SUCCESS);

	this->state_ = IPC_FREE;

	return err;
}

error_code task::ipc_state::send_extended_items(thread* to)
{
	auto acceptor = to->ipc_state_.get_acceptor();

	auto from = cur_thread.get();
	auto tag = from->ipc_state_.get_message_tag();

	for (size_t idx = tag.untyped_count() + 1; idx < tag.typed_count();)
	{
		auto mr = from->ipc_state_.get_mr(idx);

		if (static_cast<ipc::message_item_types>(mr & 0xF) == ipc::message_item_types::MAP)
		{
			auto map = from->ipc_state_.get_typed_item<ipc::map_item>(idx);
			auto[send, receive] = acceptor.get_send_receive_region(map.page(), map.base());

			copy_mrs_to_locked(to, idx++, 1);
			copy_mrs_to_locked(to, idx++, 1);

//			auto ret = vmm::mm_fpage_map(from->get_mm(), to->get_mm(), send, receive, nullptr);
			auto ret = from->address_space()->fpage_grant(to->address_space(), send, receive);
			if (has_error(ret))
			{
				KDEBUG_GENERALPANIC(get_error_code(ret));
			}
		}
		else if (static_cast<ipc::message_item_types>(mr & 0xF) == ipc::message_item_types::GRANT)
		{
			auto grant = from->ipc_state_.get_typed_item<ipc::grant_item>(idx);
			auto[send, receive] = acceptor.get_send_receive_region(grant.page(), grant.base());

			copy_mrs_to_locked(to, idx++, 1);
			copy_mrs_to_locked(to, idx++, 1);

//			auto ret = vmm::mm_fpage_grant(from->get_mm(), to->get_mm(), send, receive, nullptr);
//
//			if (!ret)
//			{
//				KDEBUG_GENERALPANIC(ret);
//			}
			auto ret = from->address_space()->fpage_grant(to->address_space(), send, receive);
			if (has_error(ret))
			{
				KDEBUG_GENERALPANIC(get_error_code(ret));
			}
		}
	}

	return ERROR_SUCCESS;
}

error_code task::ipc_state::send_locked(thread* to, const deadline& ddl)
{
	this->state_ = IPC_SENDING;

	error_code err = ERROR_SUCCESS;
	if (to->ipc_state_.state_ != IPC_RECEIVING)
	{
		err = to->ipc_state_.sender_wait_queue_.block(wait_queue::interruptible::No, ddl);
	}

	lock_guard g{ to->lock };

	copy_mrs_to_locked(to, 0, task::ipc_state::MR_SIZE);

	if (err = send_extended_items(to);err != ERROR_SUCCESS)
	{
		return err;
	}

	receiver_wait_queue_.wake_all(false, ERROR_SUCCESS);

	this->state_ = IPC_FREE;

	return err;

}
