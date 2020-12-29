
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
{

}

bool task::job_dispatcher::kill(error_code terminate_code) noexcept
{
	return false;
}

error_code_with_result<task::job_dispatcher> task::job_dispatcher::create(uint64_t flags,
	std::shared_ptr<job_dispatcher> parent,
	task::right_type right)
{
	return error_code_with_result<task::job_dispatcher>();
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

}

error_code task::job_dispatcher::apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept
{
	return 0;
}

task::job_policy&& task::job_dispatcher::get_policy() const
{
	return job_policy();
}

error_code task::job_dispatcher::enumerate_children(task::job_enumerator* enumerator, bool recurse)
{
	return 0;
}
