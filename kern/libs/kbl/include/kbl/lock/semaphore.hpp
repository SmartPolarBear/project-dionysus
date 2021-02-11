#pragma once

#include "task/thread/thread.hpp"

namespace kbl
{

class semaphore final
{
 public:
	semaphore() = default;

	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;

	void wait();

	void signal();

 private:
	task::wait_queue wait_queue_{};

	int64_t _count_{ 0 };
	integral_atomic_ref<int64_t> count_{ _count_ };
};
}