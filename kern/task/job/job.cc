
#include <task/job.hpp>

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

#include "kbl/data/pod_list.h"
#include "../../libs/basic_io/include/builtin_text_io.hpp"

bool task::job::kill(error_code terminate_code) noexcept
{
	return false;
}

error_code_with_result<task::job> task::job::create(uint64_t flags, std::shared_ptr<job> parent, task::right_type right)
{
	return error_code_with_result<task::job>();
}

std::unique_ptr<task::job> task::job::create_root()
{
	return std::unique_ptr<job>();
}

task::job::~job()
{

}

error_code task::job::apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept
{
	return 0;
}

task::job_policy&& task::job::get_policy() const
{
	return job_policy();
}

error_code task::job::enumerate_children(task::job_enumerator* enumerator, bool recurse)
{
	return 0;
}
