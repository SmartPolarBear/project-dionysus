// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by bear on 6/21/21.
//

#pragma once

#include "debug/thread_annotations.hpp"

#include "kbl/lock/lockable.hpp"

#include "task/thread/wait_queue.hpp"

#include "ktl/atomic.hpp"

#include "gsl/gsl"

namespace lock
{
template<BasicLockable TMutex>
class condition_variable final
{
 public:
	condition_variable() = default;
	condition_variable(condition_variable&&) noexcept = default;

	~condition_variable() = default;

	condition_variable(const condition_variable&) = delete;
	condition_variable& operator=(const condition_variable&) = delete;

	void notify() noexcept TA_REQ(!task::global_thread_lock)
	{
		lock_guard g{ task::global_thread_lock };
		wait_queue_.wake_one(true, ERROR_SUCCESS);
	}

	void notify_all() noexcept TA_REQ(!task::global_thread_lock)
	{
		lock_guard g{ task::global_thread_lock };
		wait_queue_.wake_all(true, ERROR_SUCCESS);
	}

	[[nodiscard]] error_code wait(TMutex& mut) TA_REQ(mut, !task::global_thread_lock)
	{
		task::global_thread_lock.assert_not_held();
		return wait(mut, deadline::infinite());
	}

	[[nodiscard]] error_code wait(TMutex& mut, const deadline& ddl) TA_REQ(mut, !task::global_thread_lock)
	{
		lock_guard g{ task::global_thread_lock };

		mut.unlock();
		auto _ = gsl::finally([&mut]()
		{
		  mut.lock();
		});

		auto err = wait_queue_.block(task::wait_queue::interruptible::Yes, ddl);
		if (err != ERROR_SUCCESS)
		{
			return err;
		}

		return ERROR_SUCCESS;
	}

 private:
	task::wait_queue wait_queue_{};
};
}