
#include <task/job_dispatcher.hpp>

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
#include "../../libs/basic_io/include/builtin_text_io.hpp"

task::job_dispatcher::job_dispatcher(uint64_t flags, std::shared_ptr<job_dispatcher> parent, job_policy policy)
	: dispatcher<job_dispatcher, 0>(),
	  parent(std::move(parent)),
	  max_height(parent == nullptr ? JOB_MAX_HEIGHT : parent->max_height - 1),
	  status(job_status::JS_READY),
	  exit_code(ERROR_SUCCESS),
	  policy(policy)
{

}

bool task::job_dispatcher::kill(error_code terminate_code) noexcept
{
	return false;
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
	auto root = std::shared_ptr<job_dispatcher>{
		new(&ck) job_dispatcher{ 0, nullptr, job_policy::creat_root_policy() }};

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	return root;
}

task::job_dispatcher::~job_dispatcher()
{
	remove_from_job_tree();
}

void task::job_dispatcher::remove_from_job_tree() TA_EXCL(this->lock)
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

error_code task::job_dispatcher::enumerate_children(task::job_enumerator* enumerator, bool recurse)
{
	return 0;
}

void task::job_dispatcher::remove_child_job(task::job_dispatcher* j)
{
	bool suicide = false;

	// lock scope
	{
		ktl::mutex::lock_guard guard{ lock };


	}
}

bool task::job_dispatcher::add_child_job(std::shared_ptr<task::job_dispatcher> child)
{
	return false;
}
bool task::job_dispatcher::is_ready_for_dead_transition() TA_REQ(lock)
{
	return false;
}
bool task::job_dispatcher::finish_dead_transition() TA_EXCL(lock)
{
	return false;
}


