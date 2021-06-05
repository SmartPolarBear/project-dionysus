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

namespace memory
{

class address_space_segment final
{

};

class address_space final
	: public object::kernel_object<address_space>
{
 public:

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

	error_code_with_result<address_space> duplicate();

	error_code resize(uintptr_t addr, size_t len);

	address_space_segment* intersect_vma(uintptr_t start, uintptr_t end);

 private:
	uintptr_t uheap_begin{ 0 };
	uintptr_t uheap_end{ 0 };

	vmm::pde_ptr_t pgdir{ nullptr };

};
}
