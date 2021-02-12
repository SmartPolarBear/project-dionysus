
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

task::job::job(uint64_t flags, std::shared_ptr<job> parent, job_policy _policy)
	: dispatcher<job, 0>(),
	  policy(std::move(_policy)),
	  parent(std::move(parent)),
	  ret_code(TASK_RETCODE_NORMAL),
	  status(job_status::READY),
	  max_height(parent == nullptr ? JOB_MAX_HEIGHT : parent->max_height - 1)
{

}

error_code_with_result<std::shared_ptr<task::job>> task::job::create(uint64_t flags,
	std::shared_ptr<job> parent)
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
	parent->remove_child_job(this);
}

void task::job::apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept
{
	for (auto&& p:policies)
	{
		this->policy.apply(p);
	}
}

task::job_policy task::job::get_policy() const
{
	lock::lock_guard guard{ lock };
	return policy;
}

bool task::job::add_child_job(std::shared_ptr<job> child)
{
	return false;
}

void task::job::remove_child_job(task::job* jb)
{
	bool suicide = false;

	// lock scope
	{
		lock_guard guard{ lock };

		auto iter = ktl::find_if(child_jobs.begin(), child_jobs.end(), [jb](auto ele)
		{
		  return ele->get_koid() == jb->get_koid();
		});

		if (iter == child_jobs.end())
		{
			return;
		}

		child_jobs.erase(iter);
		suicide = is_ready_for_dead_transition();
	}

	if (suicide)
	{
		finish_dead_transition();
	}
}

void task::job::remove_child_job(std::shared_ptr<job> jb)
{
	remove_child_job(jb.get());
}

bool task::job::is_ready_for_dead_transition() TA_REQ(lock)
{
	return status == job_status::KILLING && child_jobs.empty() && child_processes.empty();
}

bool task::job::finish_dead_transition() TA_EXCL(lock)
{
	// there must be big problem is parent_ die before its successor jobs
	KDEBUG_ASSERT(parent == nullptr || parent->get_status() != job_status::DEAD);

	// locked scope
	{
		lock::lock_guard guard{ lock };
		status = job_status::DEAD;
	}

	remove_from_job_trees();

	return false;
}

bool task::job::kill(task_return_code terminate_code) noexcept
{
	return false;
}

template<>
size_t task::job::get_count<task::job()>() TA_REQ(lock)
{
	return this->child_jobs.size();
}

void task::job::add_child_process(std::shared_ptr<process> proc)
{
	lock::lock_guard guard{ lock };

	this->child_processes.insert(child_processes.begin(), proc);
}

void task::job::remove_child_process(std::shared_ptr<process> proc)
{
	remove_child_process(proc.get());
}

void task::job::remove_child_process(task::process* proc)
{
	bool should_die = false;
	{
		lock_guard guard{ lock };

		auto iter = ktl::find_if(child_processes.begin(), child_processes.end(), [proc](auto p)
		{
		  return p->id == proc->id;
		});

		if (iter == child_processes.end())
		{
			return;
		}

		child_processes.erase(iter);

		should_die = is_ready_for_dead_transition();
	}

	if (should_die)
	{
		finish_dead_transition();
	}
}

template<>
size_t task::job::get_count<task::process()>() TA_REQ(lock)
{
	return this->child_processes.size();
}
