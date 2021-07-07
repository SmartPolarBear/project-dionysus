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
#include "round.hpp"

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
	size_t size_total{ sizeof(ramdisk_header) + sizeof(ramdisk_item) * items.size() };
	uint64_t sum{ 0 };

	auto header = reinterpret_cast<ramdisk_header*>(buf.get());
	auto rditems = reinterpret_cast<ramdisk_item*>(buf.get() + sizeof(ramdisk_header));

	for (const auto& item : items)
	{
		cout << "Proceeding " << item.string() << endl;

		auto fsize = file_size(item);

		strncpy(rditems->name, item.filename().c_str(), item.filename().string().size());

		rditems->offset = size_total;
		size_total += roundup(fsize, sizeof(uint64_t));
	}

	span<uint64_t> header_qwords{ (uint64_t*)buf.get(), sizeof(ramdisk_header) + sizeof(ramdisk_item) * items.size() };

	for (const auto& qw:header_qwords)
	{
		sum += qw;
	}

	for (const auto& p:items)
	{
		auto size = file_size(p);
		auto rbuf = make_unique<char[]>(roundup(size, sizeof(uint64_t)));

		ifstream ifs{ p, ios::binary };

		auto _2 = gsl::finally([&ifs]
		{
		  if (ifs.is_open())
		  {
			  ifs.close();
		  }
		});

		try
		{
			ifs.read(reinterpret_cast<char*>(rbuf.get()), size);

			span<uint64_t> fqwords{ (uint64_t*)rbuf.get(), roundup(size, sizeof(uint64_t)) / sizeof(uint64_t) };
			for (const auto& qw:fqwords)
			{
				sum += qw;
			}

			if (!ifs)
			{
				cout << "Error reading " << p << endl;
				return std::nullopt;
			}
		}
		catch (const std::exception& e)
		{
			cout << e.what() << endl;
			return std::nullopt;
		}
	}

	return make_tuple(header, size_total, sum);
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


