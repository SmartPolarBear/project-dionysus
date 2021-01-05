#pragma once
#include "task/task_dispatcher.hpp"

#include "debug/thread_annotations.hpp"

#include "ktl/span.hpp"
#include "ktl/list.hpp"

#include "kbl/atomic/atomic.hpp"

//FIXME

#include "system/scheduler.h"

namespace task
{
	class job_dispatcher;
	class thread_dispatcher;

	class process_dispatcher final
		: public dispatcher<process_dispatcher, 0>,
		  public kbl::linked_list_base<process_dispatcher*>
	{
	 public:
		static constexpr size_t PROC_MAX_NAME_LEN = 64;

		enum class [[clang::enum_extensibility(closed)]]Status
		{
			INITIAL,
			RUNNING,
			DYING,
			DEAD,
		};

	 public:
		friend class job_dispatcher;

		static kbl::integral_atomic<pid_type> pid_counter;
		static_assert(kbl::integral_atomic<pid_type>::is_always_lock_free);

		static error_code_with_result<std::shared_ptr<process_dispatcher>> create(const char* name,
			std::shared_ptr<job_dispatcher> parent);

		process_dispatcher() = delete;
		process_dispatcher(const process_dispatcher&) = delete;

		~process_dispatcher() override = default;

		error_code load_binary(uint8_t* bin, size_t binary_size, binary_types type, size_t flags);
		error_code terminate(error_code terminate_error);

		void exit(error_code code) noexcept;
		void kill(error_code code) noexcept;
		void finish_dead_transition() noexcept;

		error_code sleep(sleep_channel_type channel, lock::spinlock_struct* lk);
		error_code wakeup(sleep_channel_type channel);
		error_code wakeup_no_lock(sleep_channel_type channel);
		error_code change_heap_ptr(uintptr_t* heap_ptr);

		static constexpr size_t KERNSTACK_PAGES = 2;
		static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PAGE_SIZE;

		[[nodiscard]] process_state get_state() const
		{
			return state;
		}

		void set_state(process_state _state)
		{
			state = _state;
		}

		[[nodiscard]] pid_type get_id() const
		{
			return id;
		}

		void set_id(pid_type _id)
		{
			id = _id;
		}

		// FIXME: remove first
		vmm::mm_struct* get_mm() const
		{
			return mm;
		}

		size_t get_flags() const
		{
			return flags;
		}

		void set_flags(size_t _flags)
		{
			flags = _flags;
		}

		trap::trap_frame* get_tf() const
		{
			return tf;
		}

		/// \brief Get status of this process
		/// \return the status
		[[nodiscard]] Status get_status() const
		{
			return status;
		}

		/// \brief  status transition, must be called with held lock
		/// \param st the new status
		void set_status_locked(Status st) noexcept TA_REQ(lock);

		void kill_all_threads_locked() noexcept TA_REQ(lock);
	 private:
		process_dispatcher(std::span<char> name,
			pid_type id,
			std::shared_ptr<job_dispatcher> parent,
			std::shared_ptr<job_dispatcher> critical_to);

		error_code setup_kernel_stack();
		error_code setup_registers();
		error_code setup_mm();

		Status status;

		ktl::list<std::shared_ptr<thread_dispatcher>> threads;

		std::shared_ptr<job_dispatcher> parent;
		std::shared_ptr<job_dispatcher> critical_to;

		char _name_buf[PROC_NAME_LEN]{};
		ktl::span<char> name;


		process_state state;

		pid_type id;

		pid_type parent_id;

		size_t runs;

		std::unique_ptr<uint8_t[]> kstack;

		vmm::mm_struct* mm;

		size_t flags;

		size_t wating_state;
		error_code exit_code;

		trap::trap_frame* tf;

		::context* context;

		struct sleep_data_struct
		{
			size_t channel;
		} sleep_data{};

		struct messaging_data_struct
		{
			static constexpr size_t INTERNAL_BUF_SIZE = 64;

			lock::spinlock_struct lock;

			// message passing
			void* data;
			size_t data_size;
			uint8_t internal_buf[INTERNAL_BUF_SIZE];

			// page passing
			bool can_receive;
			void* dst;
			size_t unique_value;
			pid_type from;
			uint64_t perms;

		} messaging_data{};


		//FIXME
	 public:

		friend error_code process_load_binary(IN process_dispatcher* porc,
			IN uint8_t* bin,
			IN size_t binary_size,
			IN binary_types type,
			IN size_t flags);

		// handle task cleanup when exiting
		friend void process_exit(IN process_dispatcher* proc);

		// terminate current task
		friend error_code process_terminate(error_code error_code);

		// sleep on certain channel
		friend error_code process_sleep(size_t channel, lock::spinlock_struct* lock);

		// wake up child_processes sleeping on certain channel
		friend error_code process_wakeup(size_t channel);
		friend error_code process_wakeup_nolock(size_t channel);

		// send and receive message
		friend error_code process_ipc_send(pid_type pid, IN const void* message, size_t size);
		friend error_code process_ipc_receive(OUT void* message_out);
		// send and receive a page
		friend error_code process_ipc_send_page(pid_type pid, uint64_t unique_val, IN const void* page, size_t perm);
		friend error_code process_ipc_receive_page(OUT void* out_page);

		// allocate more memory
		friend error_code process_heap_change_size(IN process_dispatcher* proc, IN OUT uintptr_t* heap_ptr);

		friend size_t process_terminate_impl(task::process_dispatcher* proc,
			error_code err);

		friend error_code alloc_ustack(task::process_dispatcher* proc);

		friend void scheduler::scheduler_enter();

		friend void scheduler::scheduler_loop();

		friend void scheduler::scheduler_yield();
	};
}