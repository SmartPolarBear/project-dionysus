
#include <task/job/job.hpp>
#include <utility>

#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"

#include "kbl/lock/spinlock.h"
#include "kbl/checker/allocate_checker.hpp"
#include "kbl/data/pod_list.h"

#include "ktl/algorithm.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using namespace lock;

task::job::job([[maybe_unused]]uint64_t flags, const std::shared_ptr<job>& parent, job_policy _policy)
	: solo_dispatcher<job, 0>(),
	  policy_(std::move(_policy)),
	  parent_(parent),
	  ret_code_(TASK_RETCODE_NORMAL),
	  status_(job_status::READY),
	  max_height_(parent == nullptr ? JOB_MAX_HEIGHT : parent->max_height_ - 1)
{

}

error_code_with_result<std::shared_ptr<task::job>> task::job::create(uint64_t flags,
	const std::shared_ptr<job>& parent)
{
	if (parent != nullptr && parent->get_max_height() == 0)
	{
		return -ERROR_OUT_OF_BOUND;
	}

	kbl::allocate_checker ck{};
	std::shared_ptr<task::job> ret{ new(&ck) job{ flags, parent, parent->get_policy() }};
	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	return ret;
}

error_code_with_result<std::shared_ptr<task::job>> task::job::create_root()
{
	kbl::allocate_checker ck{};

	std::shared_ptr<job> root{ new(&ck) job{ 0, nullptr, job_policy::creat_root_policy() }};
	if (!ck.check())
	{
		KDEBUG_GERNERALPANIC_CODE(-ERROR_MEMORY_ALLOC);
	}

	return root;
}

task::job::~job()
{
	remove_from_job_trees();
}

void task::job::remove_from_job_trees() TA_EXCL(this->lock)
{
	auto parent = parent_.lock();
	parent->remove_child_job(this);
}

void task::job::apply_basic_policy(std::span<policy_item> policies) noexcept
{
	for (auto&& p:policies)
	{
		this->policy_.apply(p);
	}
}

task::job_policy task::job::get_policy() const
{
	lock::lock_guard guard{ lock };
	return policy_;
}

bool task::job::add_child_job(job* child)
{
	canary_.assert();

	lock_guard g{ lock };

	if (status_ != job_status::READY)
		return false;

	KDEBUG_ASSERT(child->job_link.is_empty_or_detached());

	child_jobs_.push_back(child);

	return true;
}

void task::job::remove_child_job(task::job* jb)
{
	bool suicide = false;

	// lock scope
	{
		lock_guard guard{ lock };

		auto iter = ktl::find_if(child_jobs_.begin(), child_jobs_.end(), [jb](auto& ele)
		{
		  return ele.get_koid() == jb->get_koid();
		});

		if (iter == child_jobs_.end())
		{
			return;
		}

		child_jobs_.erase(iter);
		suicide = is_ready_for_dead_transition_locked();
	}

	if (suicide)
	{
		finish_dead_transition_unlocked();
	}
}

bool task::job::is_ready_for_dead_transition_locked() TA_REQ(lock)
{
	return status_ == job_status::KILLING && child_jobs_.empty() && child_processes_.empty();
}

bool task::job::finish_dead_transition_unlocked()
{
	{
		auto parent = parent_.lock();
		// there must be big problem is parent_ die before its successor jobs
		KDEBUG_ASSERT(parent == nullptr || parent->get_status() != job_status::DEAD);
	}

	// locked scope
	{
		lock::lock_guard guard{ lock };
		status_ = job_status::DEAD;
	}

	remove_from_job_trees();

	return false;
}

bool task::job::kill(task_return_code code) noexcept
{
	canary_.assert();

	bool should_die = false;
	{
		lock_guard g{ lock };

		if (status_ != job_status::READY)
			return false;

		ret_code_ = code;
		status_ = job_status::KILLING;

		while (!child_jobs_.empty())
		{
			auto top = child_jobs_.front_ptr();
			child_jobs_.pop_front();

			top->kill(code);
		}

		while (!child_processes_.empty())
		{
			auto top = child_processes_.front_ptr();
			child_processes_.pop_front();

			top->kill(code);
		}

		should_die = is_ready_for_dead_transition_locked();
	}

	if (should_die)
	{
		finish_dead_transition_unlocked();
	}

	return true;
}

template<>
size_t task::job::get_count_locked<task::job>() TA_REQ(lock)
{
	return this->child_jobs_.size();
}

template<>
size_t task::job::get_count_locked<task::process>() TA_REQ(lock)
{
	return this->child_processes_.size();
}

void task::job::add_child_process(process* proc)
{
	lock::lock_guard guard{ lock };

	this->child_processes_.insert(child_processes_.begin(), proc);
}

void task::job::remove_child_process(task::process* proc)
{
	bool should_die = false;
	{
		lock_guard guard{ lock };

		auto iter = ktl::find_if(child_processes_.begin(), child_processes_.end(), [proc](auto& p)
		{
		  return p.get_koid() == proc->get_koid();
		});

		if (iter == child_processes_.end())
		{
			return;
		}

		child_processes_.erase(iter);

		should_die = is_ready_for_dead_transition_locked();
	}

	if (should_die)
	{
		finish_dead_transition_unlocked();
	}
}
