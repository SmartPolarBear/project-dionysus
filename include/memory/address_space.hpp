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
// Created by bear on 6/5/21.
//

#pragma once

#include "object/kernel_object.hpp"

#include "memory/fpage.hpp"

#include "system/vmm.h"

#include "kbl/data/list.hpp"

namespace memory
{

class address_space_segment final
{
 public:
	friend class address_space;
	using link_type = kbl::list_link<address_space_segment, lock::spinlock>;

	[[nodiscard]] address_space_segment(uintptr_t vm_start, uintptr_t vm_end, uint64_t vm_flags);
	~address_space_segment();
	address_space_segment(address_space_segment&& another);

	address_space_segment(const address_space_segment&) = delete;
	address_space_segment& operator=(const address_space_segment&) = delete;

	error_code resize(uintptr_t start, uintptr_t end);

	[[nodiscard]] uintptr_t start() const
	{
		return this->start_;
	}

	[[nodiscard]] uintptr_t end() const
	{
		return end_;
	}

	[[nodiscard]] uint64_t flags() const
	{
		return flags_;
	}

 private:

	uintptr_t start_{ 0 };
	uintptr_t end_{ 0 };

	uint64_t flags_{ 0 };

	class address_space* parent_{ nullptr };

	link_type link_{ this };
};

class address_space final
	: public object::kernel_object<address_space>
{
 public:
	using segment_list_type = kbl::intrusive_list_with_default_trait<address_space_segment,
	                                                                 lock::spinlock,
	                                                                 &address_space_segment::link_,
	                                                                 true>;

	address_space();
	address_space(address_space&& another);
	~address_space();

	address_space(const address_space&) = delete;
	address_space& operator=(const address_space&) = delete;

	error_code_with_result<address_space_segment*> map(uintptr_t addr, size_t len, uint64_t flags);

	error_code_with_result<address_space_segment*> mm_fpage_map(address_space* to,
		const task::ipc::fpage& send,
		const task::ipc::fpage& receive);

	error_code_with_result<address_space_segment*> mm_fpage_grant(address_space* to,
		const task::ipc::fpage& send,
		const task::ipc::fpage& receive);

	error_code unmap(uintptr_t addr, size_t len);

	error_code_with_result<address_space*> duplicate();

	error_code resize(uintptr_t addr, size_t len);

	void insert_vma(address_space_segment* vma);

	address_space_segment* find_vma(uintptr_t addr);

	address_space_segment* intersect_vma(uintptr_t start, uintptr_t end);

 private:
	void assert_segment_overlap(address_space_segment* prev, address_space_segment* next);

	uintptr_t uheap_begin{ 0 };
	uintptr_t uheap_end{ 0 };

	segment_list_type segments{};

	address_space_segment* search_cache_{ nullptr };

	vmm::pde_ptr_t pgdir_{ nullptr };

	lock::spinlock lock_{ "addr_space" };

};

}