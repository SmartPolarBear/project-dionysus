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
// Created by bear on 7/13/21.
//

#include <ramdisk.hpp>

#include "include/ramdisk.hpp"

#include "debug/kdebug.h"

#include "system/types.h"
#include "system/error.hpp"
#include "system/multiboot.h"

#include "task/job/job.hpp"
#include "task/process/process.hpp"
#include "task/thread/thread.hpp"

#include "../libs/basic_io/include/builtin_text_io.hpp"

#include "ktl/span.hpp"

using namespace ktl;

static inline constexpr auto RAMDISK_CMDLINE = "/bootramdisk";

extern std::shared_ptr<task::job> root_job;

static inline void run(string_view name, uint8_t* buf, size_t size)
{
	auto create_ret = task::process::create(name.data(), buf, size, root_job);
	if (has_error(create_ret))
	{
		KDEBUG_GERNERALPANIC_CODE(get_error_code(create_ret));
	}

	auto proc = get_result(create_ret);

	write_format("[cpu %d]load binary: %s\n", cpu->id, name);
}

static inline ramdisk_header* find_ramdisk()
{
	uint8_t* bin = nullptr;
	size_t size = 0;
	auto ret = multiboot::find_module_by_cmdline(RAMDISK_CMDLINE, &size, &bin);

	KDEBUG_ASSERT_MSG(ret == ERROR_SUCCESS, "Cannot find boot ramdisk.");

	return reinterpret_cast<ramdisk_header*>(bin);
}

bool verify_checksum(const ramdisk_header* header)
{
	auto size = header->size;
	uint64_t sum = 0;
	span<const uint64_t>
		qwords{ reinterpret_cast<const uint64_t*>(header), roundup(size, sizeof(uint64_t)) / sizeof(uint64_t) };

	for (const auto& qw:qwords)
	{
		sum += qw;
	}

	return sum == 0;
}

enum class test
{
	A, B, C
};

error_code init::load_boot_ramdisk()
{

	auto header = find_ramdisk();

	write_format("Load system component from ramdisk %s\n", header->name);
	write_format("Ramdisk size: %lld, count of items: %lld. \n", header->size, header->count);

	// TODO: check header parameters

	KDEBUG_ASSERT_MSG(verify_checksum(header), "Invalid boot ramdisk with wrong checksum.");

	span<ramdisk_item> items{ header->items, header->count };

	for (const auto& item:items)
	{
		run(item.name, ((uint8_t*)header) + item.offset, item.size);
	}

	return ERROR_SUCCESS;
}