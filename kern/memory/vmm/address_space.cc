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
#include "memory/pmm.hpp"

#include "system/memlayout.h"
#include "system/mmu.h"

#include <utility>

#include "kbl/checker/allocate_checker.hpp"

using namespace memory;
using namespace vmm;

using namespace kbl;
using namespace lock;

address_space_segment::address_space_segment(uintptr_t vm_start, uintptr_t vm_end, uint64_t vm_flags)
	: start_(vm_start), end_(vm_end), flags_(vm_flags)
{
	KDEBUG_ASSERT(start_ < end_);
}

address_space_segment::address_space_segment(address_space_segment&& another)
	: start_(std::exchange(another.start_, 0)),
	  end_(std::exchange(another.end_, 0)),
	  flags_(std::exchange(another.flags_, 0))
{
	KDEBUG_ASSERT(start_ < end_);
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
	: uheap_begin_(std::exchange(another.uheap_begin_, 0)),
	  uheap_end_(std::exchange(another.uheap_end_, 0)),
	  pgdir_(std::exchange(another.pgdir_, nullptr))
{
	for (auto& seg:another.segments)
	{
		segments.push_back(seg);
	}

	another.segments.clear();
}

address_space::~address_space()
{
}

error_code_with_result<address_space_segment*> address_space::map(uintptr_t addr, size_t len, uint64_t flags)
{
	lock_guard g{ lock_ };

	uintptr_t start = rounddown(addr, PAGE_SIZE), end = roundup(addr + len, PAGE_SIZE);

	end = std::min(end, USER_TOP);

	if (!VALID_USER_REGION(start, end))
	{
		return -ERROR_INVALID;
	}

	address_space_segment* vma = nullptr;
	if ((vma = find_vma(start)) != nullptr && end > vma->start())
	{
		// the vma exists
		return -ERROR_ALREADY_EXIST;
	}
	else
	{
		flags &= ~VM_SHARE;

		kbl::allocate_checker ck{};
		vma = new(&ck) address_space_segment(start, end, flags);

		KDEBUG_ASSERT(uintptr_t(& vma) != uintptr_t (&ck));

		if (!ck.check())
		{
			return -ERROR_MEMORY_ALLOC;
		}

		insert_vma(vma);

	}

	return vma;
}

error_code_with_result<address_space_segment*> address_space::mm_fpage_map(address_space* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive)
{

	uint32_t flags = VM_SHARE;

	if (send.check_rights(task::ipc::AR_W))flags |= VM_WRITE;
	if (send.check_rights(task::ipc::AR_R))flags |= VM_READ;
	if (send.check_rights(task::ipc::AR_X))flags |= VM_EXEC;

	// Ensure the VMA exist
	auto ret = to->map(send.get_base_address(), send.get_size(), flags);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	// Map the corresponding address

	for (auto start = send.get_base_address(); start + PAGE_SIZE <= send.get_base_address() + send.get_size();
	     start += PAGE_SIZE)
	{
		auto pde = vmm::walk_pgdir(pgdir_, start, false);

		auto to_pde = vmm::walk_pgdir(to->pgdir_, start - send.get_base_address() + receive.get_base_address(), true);

		*to_pde = *pde;

		memory::physical_memory_manager::instance()->flush_tlb(to->pgdir_, start);
	}

	return ERROR_SUCCESS;
}

error_code_with_result<address_space_segment*> address_space::mm_fpage_grant(address_space* to,
	const task::ipc::fpage& send,
	const task::ipc::fpage& receive)
{
	uint32_t flags = VM_SHARE;
	if (send.check_rights(task::ipc::AR_W))flags |= VM_WRITE;
	if (send.check_rights(task::ipc::AR_R))flags |= VM_READ;
	if (send.check_rights(task::ipc::AR_X))flags |= VM_EXEC;

	// Ensure the target is send
	auto ret = to->map(send.get_base_address(), send.get_size(), flags);
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	// Remove the vma from the source
	ret = unmap(send.get_base_address(), send.get_size());
	if (has_error(ret))
	{
		return get_error_code(ret);
	}

	// do real map and unmap

	for (auto start = send.get_base_address(); start + PAGE_SIZE <= send.get_base_address() + send.get_size();
	     start += PAGE_SIZE)
	{
		auto pde = vmm::walk_pgdir(pgdir_, start, false);

		auto to_pde = vmm::walk_pgdir(to->pgdir_, start - send.get_base_address() + receive.get_base_address(), true);

		*to_pde = *pde;

		*pde = 0;

		memory::physical_memory_manager::instance()->flush_tlb(pgdir_, start);

		memory::physical_memory_manager::instance()->flush_tlb(to->pgdir_, start);
	}

	return ERROR_SUCCESS;

}

error_code address_space::unmap(uintptr_t addr, size_t len)
{
	uintptr_t start = PAGE_ROUNDDOWN(addr), end = PAGE_ROUNDUP(addr + len);
	if (!VALID_USER_REGION(start, end))
	{
		return -ERROR_INVALID;
	}

	auto vma = find_vma(start);
	if (vma == nullptr || end < vma->start_)
	{
		return ERROR_SUCCESS; // no need to remove
	}

	if (vma->start_ < start && end < vma->end_)
	{
		//           range to remove
		//    [       [***********]       ]
		//     |------|      ^    |-------|
		//	  Create new     |     Shrink old
		//                   |
		//                 unmap
		kbl::allocate_checker ck{};
		auto new_vma = new(&ck) address_space_segment(vma->start_, start, vma->flags_);

		KDEBUG_ASSERT(uintptr_t(& new_vma) != uintptr_t (&ck));

		if (!ck.check())
		{
			return -ERROR_MEMORY_ALLOC;
		}

		auto ret = vma->resize(end, vma->end_);
		if (ret != ERROR_SUCCESS)
		{
			return ret;
		}

		insert_vma(new_vma);

		unmap_range(pgdir_, start, end);

		return ERROR_SUCCESS;
	}

	segment_list_type freelist{};

	for (segment_list_type::iterator_type iter{ &vma->link_ }; iter != segments.end(); iter++)
	{
		if (iter->start_ >= end)
		{
			break;
		}

		freelist.push_back(&(*iter));
	}

	for (auto& entry:freelist)
	{
		segments.remove(entry);

		uintptr_t unmap_start = entry.start(), unmap_end = entry.end();

		if (entry.start() < start)
		{
			unmap_start = start;
			entry.resize(entry.start_, start);
			insert_vma(&entry);
		}
		else
		{
			if (end < entry.end())
			{
				unmap_end = end;
				entry.resize(end, entry.end());
				insert_vma(&entry);
			}
			else
			{
				delete &entry;
			}
		}
		unmap_range(pgdir_, unmap_start, unmap_end);
	}

	return ERROR_SUCCESS;
}

error_code_with_result<address_space*> address_space::duplicate()
{
	allocate_checker ck{};
	auto to = new(&ck) address_space{};
	KDEBUG_ASSERT(uintptr_t(& ck)!=uintptr_t(to));

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	for (auto& seg:segments)
	{
		auto new_seg = new(&ck) address_space_segment(seg.start_, seg.end_, seg.flags_);
		KDEBUG_ASSERT(uintptr_t(& ck)!=uintptr_t(new_seg));

		if (!ck.check())
		{
			return -ERROR_MEMORY_ALLOC;
		}

		to->insert_vma(new_seg);

		copy_range(pgdir_, to->pgdir_, seg.start_, seg.end_);
	}

	return to;
}

error_code address_space::resize(uintptr_t addr, size_t len)
{
	uintptr_t start = PAGE_ROUNDDOWN(addr), end = PAGE_ROUNDUP((addr + len));
	if (!VALID_USER_REGION(start, end))
	{
		return -ERROR_INVALID;
	}

	error_code ret = ERROR_SUCCESS;

	if ((ret = unmap(start, end - start)) != ERROR_SUCCESS)
	{
		return ret;
	}

	constexpr auto VM_FLAGS = VM_READ | VM_WRITE;

	auto vma = find_vma(start - 1);
	if (vma != nullptr && vma->end_ == start && vma->flags_ == VM_FLAGS)
	{
		vma->end_ = end;
		return ERROR_SUCCESS;
	}

	kbl::allocate_checker ck{};
	vma = new(&ck) address_space_segment(start, end, VM_FLAGS);

	// FIXME: remove this later
	KDEBUG_ASSERT(uintptr_t(& vma) != uintptr_t (&ck));

	if (!ck.check())
	{
		return -ERROR_MEMORY_ALLOC;
	}

	insert_vma(vma);

	return ERROR_SUCCESS;
}

void address_space::insert_vma(address_space_segment* vma)
{
	address_space_segment* prev = nullptr;
	segment_list_type::iterator_type prev_iter;
	for (auto iter = segments.begin(); iter != segments.end(); iter++)
	{
		if (iter->start_ > vma->start_)
		{
			break;
		}
		prev = &(*iter);
		prev_iter = iter;
	}

	auto next = prev->link_.next_->parent_;

	assert_segment_overlap(prev, vma);
	assert_segment_overlap(vma, next);

	segments.insert(prev_iter, vma);

	vma->parent_ = this;
}

address_space_segment* address_space::find_vma(uintptr_t addr)
{
	auto ret = search_cache_;

	if (!(ret != nullptr &&
		ret->start() <= addr &&
		ret->end() > addr))
	{
		ret = nullptr;
		for (auto& seg:segments)
		{
			if (seg.start() <= addr && seg.end() > addr)
			{
				ret = &seg;
				break;
			}
		}
	}

	if (ret != nullptr)
	{
		search_cache_ = ret;
	}

	return ret;
}

address_space_segment* address_space::intersect_vma(uintptr_t start, uintptr_t end)
{
	auto vma = find_vma(start);
	if (vma != nullptr && end <= vma->start_)
	{
		return nullptr;
	}
	return vma;
}

void address_space::assert_segment_overlap(address_space_segment* prev, address_space_segment* next)
{
	KDEBUG_ASSERT(prev->start_ < prev->end_);
	KDEBUG_ASSERT(prev->end_ <= next->start_);
	KDEBUG_ASSERT(next->start_ < next->end_);
}
