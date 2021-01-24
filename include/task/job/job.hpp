#pragma once

#include "object/dispatcher.hpp"

#include "arch/amd64/cpu/regs.h"

#include "debug/thread_annotations.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"
#include "kbl/data/list_base.hpp"
#include "kbl/ref_count/ref_count_base.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/list.hpp"
#include "ktl/concepts.hpp"

#include "task/process/process.hpp"

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
	 public:
		friend class process;
		friend class thread_dispatcher;

		using job_list_type = ktl::list<std::shared_ptr<job>>;
		using process_list_type = ktl::list<std::shared_ptr<process>>;

		template<typename T>
		using array_of_child = ktl::unique_ptr<T[]>();

		static constexpr size_t JOB_NAME_MAX = 64;
		static constexpr size_t JOB_MAX_HEIGHT = 16;
	 public:

		[[nodiscard]]bool kill(task_return_code terminate_code) noexcept;

		void apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept;

		[[nodiscard]]job_policy get_policy() const;

		[[nodiscard]]static error_code_with_result<std::shared_ptr<task::job>> create_root();

		[[nodiscard]]static error_code_with_result<std::shared_ptr<task::job>> create(uint64_t flags,
			std::shared_ptr<job> parent);

		~job() final;

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
		job(uint64_t flags,
			std::shared_ptr<job> parent,
			job_policy policy);

		void remove_from_job_trees() TA_EXCL(lock);

		bool add_child_job(std::shared_ptr<job> child);
		void remove_child_job(job* job);
		void remove_child_job(std::shared_ptr<job> proc);

		void add_child_process(std::shared_ptr<process> proc);
		void remove_child_process(process* proc);
		void remove_child_process(std::shared_ptr<process> proc);

		bool is_ready_for_dead_transition()TA_REQ(lock);
		bool finish_dead_transition()TA_EXCL(lock);

		template<typename T>
		[[nodiscard]]size_t get_count() TA_REQ(lock);

//		template<typename TChildrenList, typename TChild, typename TFunc>
//		requires ktl::ListOfTWithBound<TChildrenList, TChild> && (!ktl::Pointer<TChild>)
//		[[nodiscard]]error_code_with_result<ktl::unique_ptr<TChild* []>> for_each_child(TChildrenList& children,
//			TFunc func) TA_REQ(lock);

	 private:
		job_list_type child_jobs;
		process_list_type child_processes;

		job_policy policy;

		std::shared_ptr<job> parent;

		char name[JOB_NAME_MAX]{ 0 };

		bool killed{ false };

		task_return_code ret_code;

		job_status status TA_GUARDED(lock);

		const size_t max_height;
	};
}