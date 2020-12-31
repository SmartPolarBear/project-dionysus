#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/cpu/regs.h"

#include "debug/thread_annotations.hpp"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "ktl/concepts.hpp"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"

#include "kbl/lock/spinlock.h"
#include "kbl/data/pod_list.h"
#include "kbl/data/list_base.hpp"
#include "kbl/atomic/atomic_ref.hpp"
#include "kbl/ref_count/ref_count_base.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/unique_ptr.hpp"

#include "task/process_dispatcher.hpp"
#include "task/dispatcher.hpp"

#include <cstring>
#include <algorithm>
#include <span>
#include <optional>

namespace task
{
	enum class policy_action
	{
		// for the sake of debugging, starting from 1
		kPolicyAllow = 1,
		kPolicyDeny,
		kPolicyKill,
		kPolicyAllowWithException,
		kPolicyDenyWithException,
	};

	enum class policy_condition : size_t
	{
		// TODO: define them
		kPlaceholder,

		kPolicyConditionMax
	};

	struct policy_item
	{
		policy_condition condition;
		policy_action action;
	};

	class job_policy
	{
	 public:
		static constexpr size_t POLICY_CONDITION_MAX = static_cast<size_t >(policy_condition::kPolicyConditionMax);

		static job_policy creat_root_policy()
		{
			//TODO
			return job_policy();
		}

		[[nodiscard]]std::span<std::optional<policy_item>, POLICY_CONDITION_MAX> get_policies()
		{
			return { action };
		}

		[[nodiscard]] std::optional<policy_item> get_for_condition(policy_condition cond)
		{
			return action[static_cast<size_t>(cond)];
		}

		void apply(policy_item& policy)
		{
			action[static_cast<size_t>(policy.condition)] = policy;
		}

		void merge_with(job_policy&& another)
		{
			for (std::optional<policy_item>& action:another.action)
			{
				if (action.has_value())
				{
					apply(action.value());
				}
			}
		}

	 private:
		std::optional<policy_item> action[POLICY_CONDITION_MAX];
	};

	class job_dispatcher;

	class job_enumerator
	{
	 public:
		virtual bool on_job(job_dispatcher* jb)
		{
			return true;
		}

		virtual bool on_process(process_dispatcher* pr)
		{
			return true;
		}
	 protected:
		virtual ~job_enumerator() = default;
	};

	enum class job_status
	{
		JS_READY,
		JS_KILLING,
		JS_DEAD
	};

	class job_dispatcher final
		: public dispatcher<job_dispatcher, 0>,
		  public kbl::single_linked_child_list_base<job_dispatcher*>
	{
	 public:
		using job_list_type = kbl::single_linked_child_list_base<job_dispatcher*>;
		using process_list_type = kbl::single_linked_child_list_base<process_dispatcher*>;

		template<typename T>
		using array_of_child = ktl::unique_ptr<T[]>();

		static constexpr size_t JOB_NAME_MAX = 64;
		static constexpr size_t JOB_MAX_HEIGHT = 32;
	 public:

		[[nodiscard]]bool kill(error_code terminate_code) noexcept;

		[[nodiscard]]void apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept;

		[[nodiscard]]job_policy get_policy() const;

		[[nodiscard]]error_code enumerate_children(job_enumerator* enumerator, bool recurse);

		static error_code_with_result<std::shared_ptr<task::job_dispatcher>> create_root();

		static error_code_with_result<std::shared_ptr<task::job_dispatcher>> create(uint64_t flags,
			std::shared_ptr<job_dispatcher> parent);

		~job_dispatcher() final;

		[[nodiscard]]size_t get_max_height() const
		{
			return max_height;
		}

		[[nodiscard]] job_status get_status() const
		{
			ktl::mutex::lock_guard g{ this->lock };
			return status;
		}

	 private:
		job_dispatcher(uint64_t flags,
			std::shared_ptr<job_dispatcher> parent,
			job_policy policy);

		void remove_from_job_trees() TA_EXCL(lock);

		bool add_child_job(std::shared_ptr<task::job_dispatcher> child);
		void remove_child_job(job_dispatcher* j);

		bool is_ready_for_dead_transition()TA_REQ(lock);
		bool finish_dead_transition()TA_EXCL(lock);

		template<typename T>
		[[nodiscard]]size_t get_count() TA_REQ(lock);

		template<typename TChildrenList, typename TChild, typename TFunc>
		requires ktl::ListOfTWithBound<TChildrenList, TChild> && (!ktl::Pointer<TChild>)
		[[nodiscard]]error_code_with_result<ktl::unique_ptr<TChild* []>> for_each_child(TChildrenList& children,
			TFunc func);
	 private:
		job_list_type child_jobs;
		process_list_type child_processes;

		job_policy policy;

		std::shared_ptr<job_dispatcher> parent;

		char name[JOB_NAME_MAX];

		bool killed;

		error_code exit_code;

		job_status status TA_GUARDED(lock);

		const size_t max_height;
	};
}