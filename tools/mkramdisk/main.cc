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
// Created by bear on 6/26/21.
//

#include <ramdisk.hpp>

#include <iostream>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>

#include <cstring>

#include <gsl/gsl>

#include <sysexits.h>

using namespace std;
using namespace std::filesystem;

/// \brief
/// \param out_name
/// \param items
/// \return if update is needed for ramdisk
bool check_update_time(string_view out_name, span<string_view> items)
{
	path out_path{ out_name };
	auto out_md = last_write_time(out_path);

	for (const auto& i:items)
	{
		if (last_write_time(path{ i }) > out_md)
		{
			return true;
		}
	}

	return false;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "Usage: mkramdisk -o <ramdisk file>  <files...>";
	}

	span<char*> args{ argv, static_cast<std::size_t>(argc) };

	// get the output file name
	string_view out_name;
	auto out_pos = args.begin();
	for (auto iter = args.begin(); iter != args.end(); iter++)
	{
		if (strncmp(*iter, "-o", 2) == 0)
		{
			out_name = *(iter + 1);
			out_pos = iter;
			break;
		}
	}

	if (out_name.empty())
	{
		cout << "Usage: mkramdisk -o <ramdisk file>  <files...>";
		return EX_USAGE;
	}

	auto item_count = argc - 3;

	auto content = make_unique<string_view[]>(item_count);
	{
		size_t counter = 0;
		for (auto iter = args.begin(); iter != args.end(); iter++)
		{
			if (iter != out_pos && iter != out_pos + 1)
			{
				content[counter++] = *iter;
			}
		}
	}
	span<string_view> content_span{ content.get(), static_cast<size_t>(item_count) };

	cout << "Checking modified time." << endl;

	if (!check_update_time(out_name, content_span))
	{
		cout << "Everything up to date" << endl;
		return 0;
	}
	else
	{
		// clear the file
		ofstream out_file{ out_name.data(), ios::binary };
		auto _ = gsl::finally([&out_file]
		{
		  if (out_file.is_open())
		  { out_file.close(); }
		});
		out_file << 0;
	}

	cout << "Creating metadata." << endl;

	auto buf = make_unique<char[]>(sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count);

	// construct the header
	auto header = reinterpret_cast<ramdisk_header*>(buf.get());
	span<ramdisk_item> item_span
		{ reinterpret_cast<ramdisk_item*>(buf.get() + sizeof(ramdisk_header)), static_cast<size_t>(item_count) };

	header->magic = RAMDISK_HEADER_MAGIC;
	header->architecture = ARCH_AMD64;
	strncpy(header->name, "RAMDISK", 8);
	header->count = item_count;
	header->size = sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count;

	// construct item info
	ofstream out_file{ out_name.data(), ios::binary | ios::app };
	auto _ = gsl::finally([&out_file]
	{
	  if (out_file.is_open())
	  { out_file.close(); }
	});

	{
		auto item_iter = item_span.begin();
		for (const auto& i:content_span)
		{
			auto p = path{ i };
			auto fsize = file_size(p);

			{
				auto sname = p.filename().string();
				strncpy(item_iter->name, sname.data(), sname.length());
			}

			item_iter->offset = header->size;
			header->size += fsize;
		}
	}

	out_file.write(buf.get(), sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count);
	if (!out_file)
	{
		cout << "Error writing metadata." << endl;
		return EX_IOERR;
	}

	for (const auto& i:content_span)
	{
		cout << i << endl;

		auto p = path{ i };
		auto fsize = file_size(p);

		auto fbuf = make_unique<uint8_t[]>(fsize);

		ifstream ifs{ i.data(), ios::binary };

		auto ___ = gsl::finally([&ifs]
		{
		  if (ifs.is_open())
		  {
			  ifs.close();
		  }
		});

		ifs.read(reinterpret_cast<char*>(fbuf.get()), fsize);
		if (!ifs)
		{
			cout << "Error reading " << i << endl;
			return EX_IOERR;
		}

		out_file.write(reinterpret_cast<char*>(fbuf.get()), fsize);
		if (!out_file)
		{
			cout << "Error reading " << i << endl;
			return EX_IOERR;
		}

	}

	return 0;
}
