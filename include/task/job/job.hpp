#pragma once

#include "object/dispatcher.hpp"

#include "arch/amd64/cpu/regs.h"

#include "debug/thread_annotations.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"
#include "kbl/ref_count/ref_count_base.hpp"
#include "kbl/data/name.hpp"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/list.hpp"
#include "ktl/concepts.hpp"

#include "task/job/job_policy.hpp"
#include "task/process/process.hpp"

#include <cstring>
#include <algorithm>
#include <span>
#include <optional>

namespace task
{

class job final
	: public object::dispatcher<job, 0>
{
 public:
	enum class [[clang::enum_extensibility(closed)]] job_status
	{
		READY,
		KILLING,
		DEAD
	};

	using link_type = kbl::list_link<job, lock::spinlock>;

	friend class process;
	friend class thread;

	using process_list_type = kbl::intrusive_list_with_default_trait<process,
	                                                                 lock::spinlock,
	                                                                 &process::job_link,
	                                                                 true>;

	static constexpr size_t JOB_NAME_MAX = 64;
	static constexpr size_t JOB_MAX_HEIGHT = 16;
 public:

	/*can discard*/ bool kill(task_return_code terminate_code) noexcept;

	void apply_basic_policy(std::span<policy_item> policies) noexcept;

	[[nodiscard]]job_policy get_policy() const;

	[[nodiscard]]static error_code_with_result<std::shared_ptr<task::job>> create_root();

	[[nodiscard]]static error_code_with_result<std::shared_ptr<task::job>> create(uint64_t flags,
		const std::shared_ptr<job>& parent);

	~job() final;

	[[nodiscard]]size_t get_max_height() const
	{
		return max_height_;
	}

	[[nodiscard]] job_status get_status() const
	{
		lock::lock_guard g{ this->lock };
		return status_;
	}

 private:
	job([[maybe_unused]]uint64_t flags,
		const std::shared_ptr<job>& parent,
		job_policy policy);

	void remove_from_job_trees() TA_EXCL(lock);

	bool add_child_job(job* child);
	void remove_child_job(job* job);

	void add_child_process(process* proc);
	void remove_child_process(process* proc);

	bool is_ready_for_dead_transition_locked()TA_REQ(lock);
	bool finish_dead_transition_unlocked()TA_REQ(!lock);

	template<typename T>
	[[nodiscard]]size_t get_count_locked() TA_REQ(lock);

 private:
	kbl::canary<kbl::magic(" job")> canary_;

	job_policy policy_;

	ktl::weak_ptr<job> parent_;

	kbl::name<JOB_NAME_MAX> name_;

	task_return_code ret_code_;

	job_status status_ TA_GUARDED(lock);

	link_type job_link{ this };
	using job_list_type = kbl::intrusive_list_with_default_trait<job,
	                                                             lock::spinlock,
	                                                             &job::job_link,
	                                                             true>;

	job_list_type child_jobs_;
	process_list_type child_processes_;

	const size_t max_height_;

};
}