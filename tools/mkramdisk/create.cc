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
// Created by bear on 7/7/21.
//

#include <ramdisk.hpp>

#include "create.hpp"
#include "config.hpp"

#include <filesystem>
#include <span>
#include <tuple>
#include <queue>
#include <fstream>
#include <iostream>

#include <gsl/gsl>

using namespace std;
using namespace std::filesystem;

using namespace mkramdisk;
using namespace mkramdisk::configuration;

std::optional<tuple<ramdisk_header*, size_t, uint64_t>> mkramdisk::create_ramdisk(const shared_ptr<char[]>& buf,
	const vector<path>& items)
{
	size_t size_total{ 0 };
	uint64_t sum{ 0 };
	return make_tuple(reinterpret_cast<ramdisk_header*>(buf.get()), size_total, sum);
}

[[nodiscard]]bool mkramdisk::clear_target(const path& p)
{
	ofstream out_file{ p, ios::binary };
	auto _ = gsl::finally([&out_file]
	{
	  if (out_file.is_open())
	  { out_file.close(); }
	});

	try
	{
		out_file << 0;
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
		return false;
	}

	return !out_file.fail();
}


