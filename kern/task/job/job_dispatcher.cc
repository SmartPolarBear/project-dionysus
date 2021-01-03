
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
	  status(job_status::READY),
	  exit_code(ERROR_SUCCESS),
	  policy(policy)
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


void task::job_dispatcher::remove_child_job(task::job_dispatcher* j)
{
	bool suicide = false;

	// lock scope
	{
		ktl::mutex::lock_guard guard{ lock };

		if (child_jobs.find_first([](job_dispatcher* jb, const void* key)
		{
		  return jb->get_koid() == reinterpret_cast<const job_dispatcher*>(key)->get_koid();
		}, j) == nullptr)
		{
			return;
		}

		child_jobs.remove_node(static_cast<kbl::single_linked_child_list_base<job_dispatcher*>*>(j));
		suicide = is_ready_for_dead_transition();
	}

	if (suicide)
	{
		finish_dead_transition();
	}
}

bool task::job_dispatcher::add_child_job(std::shared_ptr<task::job_dispatcher> child)
{
	return false;
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

bool task::job_dispatcher::kill(error_code terminate_code) noexcept
{
	return false;
}

template<>
size_t task::job_dispatcher::get_count<task::job_dispatcher()>() TA_REQ(lock)
{
	return this->child_jobs.size();
}

template<typename TChildrenList, typename TChild, typename TFunc>
requires ktl::ListOfTWithBound<TChildrenList, TChild> && (!ktl::Pointer<TChild>)
error_code_with_result<ktl::unique_ptr<TChild* []>> task::job_dispatcher::for_each_child(TChildrenList& children,
	TFunc func) TA_REQ(lock)
{
	const size_t count = get_count<TChild>();

	if (count == 0)
	{
		return nullptr;
	}

	kbl::allocate_checker ck{};
	ktl::unique_ptr<TChild* []> ret = ktl::make_unique<TChild* []>(&ck, count);
	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	size_t idx = 0;

	TChild* iter;
	llb_for(iter, children.head)
	{
		auto err = func(*iter);
		if (err != ERROR_SUCCESS)
		{
			return err;
		}

		ret[idx++] = iter;
	}

	return ret;
}

template<>
size_t task::job_dispatcher::get_count<task::process_dispatcher()>() TA_REQ(lock)
{
	return this->child_processes.size();
}
