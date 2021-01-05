
#include <task/job_dispatcher.hpp>
#include <utility>

#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"

#include "kbl/lock/spinlock.h"
#include "kbl/checker/allocate_checker.hpp"
#include "kbl/data/pod_list.h"

#include "ktl/algorithm.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

task::job_dispatcher::job_dispatcher(uint64_t flags, std::shared_ptr<job_dispatcher> parent, job_policy _policy)
	: dispatcher<job_dispatcher, 0>(),
	  policy(std::move(_policy)),
	  parent(std::move(parent)),
	  ret_code(TASK_RETCODE_NORMAL),
	  status(job_status::READY),
	  max_height(parent == nullptr ? JOB_MAX_HEIGHT : parent->max_height - 1)
{

}

error_code_with_result<std::shared_ptr<task::job_dispatcher>> task::job_dispatcher::create(uint64_t flags,
	std::shared_ptr<job_dispatcher> parent)
{
	if (parent != nullptr && parent->get_max_height() == 0)
	{
		return -ERROR_OUT_OF_BOUND;
	}

	kbl::allocate_checker ck{};
	std::shared_ptr<task::job_dispatcher> ret{ new(&ck) job_dispatcher{ flags, parent, parent->get_policy() }};
	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	return ret;
}

error_code_with_result<std::shared_ptr<task::job_dispatcher>> task::job_dispatcher::create_root()
{
	kbl::allocate_checker ck{};

	std::shared_ptr<job_dispatcher> root{ new(&ck) job_dispatcher{ 0, nullptr, job_policy::creat_root_policy() }};
	if (!ck.check())
	{
		KDEBUG_GERNERALPANIC_CODE(-ERROR_MEMORY_ALLOC);
	}

	return root;
}

task::job_dispatcher::~job_dispatcher()
{
	remove_from_job_trees();
}

void task::job_dispatcher::remove_from_job_trees() TA_EXCL(this->lock)
{
	parent->remove_child_job(this);
}

void task::job_dispatcher::apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept
{
	for (auto&& p:policies)
	{
		this->policy.apply(p);
	}
}

task::job_policy task::job_dispatcher::get_policy() const
{
	ktl::mutex::lock_guard guard{ lock };
	return policy;
}

bool task::job_dispatcher::add_child_job(std::shared_ptr<job_dispatcher> child)
{
	return false;
}

void task::job_dispatcher::remove_child_job(task::job_dispatcher* jb)
{
	bool suicide = false;

	// lock scope
	{
		ktl::mutex::lock_guard guard{ lock };

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

void task::job_dispatcher::remove_child_job(std::shared_ptr<job_dispatcher> jb)
{

}

bool task::job_dispatcher::is_ready_for_dead_transition() TA_REQ(lock)
{
	return status == job_status::KILLING && child_jobs.empty() && child_processes.empty();
}

bool task::job_dispatcher::finish_dead_transition() TA_EXCL(lock)
{
	// there must be big problem is parent die before its successor jobs
	KDEBUG_ASSERT(parent == nullptr || parent->get_status() != job_status::DEAD);

	// locked scope
	{
		ktl::mutex::lock_guard guard{ lock };
		status = job_status::DEAD;
	}

	remove_from_job_trees();

	return false;
}

bool task::job_dispatcher::kill(task_return_code terminate_code) noexcept
{
	return false;
}

template<>
size_t task::job_dispatcher::get_count<task::job_dispatcher()>() TA_REQ(lock)
{
	return this->child_jobs.size();
}

void task::job_dispatcher::add_child_process(std::shared_ptr<process_dispatcher> proc)
{
	ktl::mutex::lock_guard guard{ lock };

	this->child_processes.insert(child_processes.begin(), proc);
}

void task::job_dispatcher::remove_child_process(std::shared_ptr<process_dispatcher> proc)
{
	child_processes.erase(ktl::find(child_processes.begin(), child_processes.end(), proc));
}

void task::job_dispatcher::remove_child_process(task::process_dispatcher* proc)
{

}

template<>
size_t task::job_dispatcher::get_count<task::process_dispatcher()>() TA_REQ(lock)
{
	return this->child_processes.size();
}
