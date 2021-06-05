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

#include "memory/address_space.hpp"

#include "system/memlayout.h"
#include "system/mmu.h"

#include <utility>

using namespace memory;
using namespace vmm;

address_space_segment::address_space_segment(uintptr_t vm_start, uintptr_t vm_end, uint64_t vm_flags)
	: start_(vm_start), end_(vm_end), flags_(vm_flags)
{
}


address_space_segment::address_space_segment(address_space_segment&& another)
	: start_(std::exchange(another.start_, 0)),
	  end_(std::exchange(another.end_, 0)),
	  flags_(std::exchange(another.flags_, 0))
{
}

address_space_segment::~address_space_segment()
{
}

error_code address_space_segment::resize(uintptr_t start, uintptr_t end)
{
	if ((start % PAGE_SIZE) != 0 || (end % PAGE_SIZE) != 0)
	{
		return -ERROR_INVALID;
	}

	if (start >= end)
	{
		return -ERROR_INVALID;
	}

	if (!(start_ <= start && end <= end_))
	{
		return -ERROR_INVALID;
	}

	start_ = start;
	end_ = end;

	return ERROR_SUCCESS;
}

address_space::address_space()
{
}

address_space::address_space(address_space&& another)
{
}

address_space::~address_space()
{
}

error_code_with_result<address_space_segment*> address_space::map(uintptr_t addr, size_t len, uint64_t flags)
{
	return error_code_with_result<address_space_segment*>();
}

error_code_with_result<address_space_segment*> address_space::mm_fpage_map(address_space* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive)
{
	return error_code_with_result<address_space_segment*>();
}

error_code_with_result<address_space_segment*> address_space::mm_fpage_grant(address_space* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive)
{
	return error_code_with_result<address_space_segment*>();
}

error_code address_space::unmap(uintptr_t addr, size_t len)
{
	return 0;
}

error_code_with_result<address_space> address_space::duplicate()
{
	return error_code_with_result<address_space>();
}

error_code address_space::resize(uintptr_t addr, size_t len)
{
	return 0;
}

void address_space::insert_vma(address_space_segment* vma)
{

}

address_space_segment* address_space::find_vma(uintptr_t addr)
{
	return nullptr;
}

address_space_segment* address_space::intersect_vma(uintptr_t start, uintptr_t end)
{
	return nullptr;
}
