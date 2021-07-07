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

#include "config.hpp"
#include "check.hpp"
#include "create.hpp"
#include "dependency.hpp"

#include <nlohmann/json.hpp>
#include <argparse/argparse.hpp>

#include <iostream>
#include <filesystem>
#include <span>
#include <string>
#include <tuple>
#include <queue>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <cstring>

#include <gsl/gsl>

#include <sysexits.h>

using namespace argparse;
using namespace nlohmann;

using namespace std;
using namespace std::filesystem;

using namespace mkramdisk;
using namespace mkramdisk::configuration;

int main(int argc, char* argv[])
{
	argparse::ArgumentParser argparse{ "mkramdisk" };

	argparse.add_argument("config")
		.help("the configuration file for mkramdisk")
		.required()
		.action([](const string& p)
		{ return path{ p }; });

	argparse.add_argument("target")
		.help("the result ramdisk file")
		.required()
		.action([](const string& p)
		{ return path{ p }; });

	// TODO: support multiple architectures
	//argparse.add_argument("arch")
	//	.help("the architecture of ramdisk");

	try
	{
		argparse.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		cout << err.what() << endl;
		cout << argparse;
		exit(EX_USAGE);
	}

	auto pth = argparse.get<path>("config");

	if (!exists(pth))
	{
		cout << "Configuration file does not exist!" << endl;
		exit(EX_IOERR);
	}

	auto target = argparse.get<path>("target");

	std::ifstream iconf(pth.string());
	json config{};
	iconf >> config;

	auto name = config["name"].get<std::string>();

	auto items = config["items"].get<std::vector<mkramdisk::configuration::item>>();

	for (const auto& item:items)
	{
		path p{ item.path() };
		if (!exists(p))
		{
			cout << item.path() << " does not exist." << endl;
			exit(EX_IOERR);
		}
	}

	if (!check_update_time(target, items))
	{
		cout << "Everything up to date" << endl;
		return 0;
	}

	auto sort_ret = sort_by_dependency(items);
	if (!sort_ret)
	{
		cout << "Can't sort by dependencies. Circular dependency detected." << endl;
		exit(EX_DATAERR);
	}

	auto paths = sort_ret.value();

	// clear the target file
	if (auto ret = clear_target(target);ret)
	{
		cout << "Can't write to target file " << target.string() << endl;
		exit(EX_IOERR);
	}

	shared_ptr<char[]> buf = make_unique<char[]>(sizeof(ramdisk_header) + sizeof(ramdisk_item) * paths.size());

	auto create_ret = create_ramdisk(buf, paths);
	if (!create_ret)
	{
		exit(EX_IOERR);
	}

	auto[header, size_total, pre_checksum]=create_ret.value();

	header->magic = RAMDISK_HEADER_MAGIC;
	header->architecture = ARCH_AMD64;
	header->checksum = ~static_cast<uint64_t>(pre_checksum) + 1ull;
	memmove(header->name, name.data(), name.size());

	cout << "Writing " << size_total << " bytes. The checksum is " << header->checksum << ". " << endl;

//	cout << "Creating metadata." << endl;
//
//	auto buf = make_unique<char[]>(sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count);
//
//	// construct the header
//	auto header = reinterpret_cast<ramdisk_header*>(buf.get());
//	span<ramdisk_item> item_span
//		{ reinterpret_cast<ramdisk_item*>(buf.get() + sizeof(ramdisk_header)), static_cast<size_t>(item_count) };
//
//	header->magic = RAMDISK_HEADER_MAGIC;
//	header->architecture = ARCH_AMD64;
//	strncpy(header->name, "RAMDISK", 8);
//	header->count = item_count;
//	header->size = sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count;
//
//	// construct item info
//	ofstream out_file{ out_name.data(), ios::binary | ios::app };
//	auto _ = gsl::finally([&out_file]
//	{
//	  if (out_file.is_open())
//	  { out_file.close(); }
//	});
//
//	{
//		auto item_iter = item_span.begin();
//		for (const auto& i:content_span)
//		{
//			auto p = path{ i };
//			auto fsize = file_size(p);
//
//			{
//				auto sname = p.filename().string();
//				strncpy(item_iter->name, sname.data(), sname.length());
//			}
//
//			item_iter->offset = header->size;
//			header->size += fsize;
//		}
//	}
//
//	out_file.write(buf.get(), sizeof(ramdisk_header) + sizeof(ramdisk_item) * item_count);
//	if (!out_file)
//	{
//		cout << "Error writing metadata." << endl;
//		return EX_IOERR;
//	}
//
//	for (const auto& i:content_span)
//	{
//		cout << i << endl;
//
//		auto p = path{ i };
//		auto fsize = file_size(p);
//
//		auto fbuf = make_unique<uint8_t[]>(fsize);
//
//		ifstream ifs{ i.data(), ios::binary };
//
//		auto ___ = gsl::finally([&ifs]
//		{
//		  if (ifs.is_open())
//		  {
//			  ifs.close();
//		  }
//		});
//
//		ifs.read(reinterpret_cast<char*>(fbuf.get()), fsize);
//		if (!ifs)
//		{
//			cout << "Error reading " << i << endl;
//			return EX_IOERR;
//		}
//
//		out_file.write(reinterpret_cast<char*>(fbuf.get()), fsize);
//		if (!out_file)
//		{
//			cout << "Error reading " << i << endl;
//			return EX_IOERR;
//		}
//
//	}

	return 0;
}
