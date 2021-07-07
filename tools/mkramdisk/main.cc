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
#include "copy.hpp"
#include "round.hpp"

#include <nlohmann/json.hpp>
#include <argparse/argparse.hpp>

#include <iostream>
#include <filesystem>
#include <span>
#include <tuple>
#include <queue>
#include <iomanip>
#include <fstream>

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

	argparse.add_argument("-c", "--config")
		.help("the configuration file for mkramdisk")
		.required()
		.action([](const string& p)
		{ return path{ p }; });

	argparse.add_argument("-t", "--target")
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

	auto pth = argparse.get<path>("--config");

	if (!exists(pth))
	{
		cout << "Configuration file does not exist!" << endl;
		exit(EX_IOERR);
	}

	auto target = argparse.get<path>("--target");

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
	if (auto ret = clear_target(target);!ret)
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
	header->size = roundup(size_total, sizeof(uint64_t));
	header->count = paths.size();
	header->checksum = ~static_cast<uint64_t>(pre_checksum) + 1ull;
	memmove(header->name, name.data(), name.size());

	try
	{
		ofstream of{ target, ios::binary | ios::app };
		auto _ = gsl::finally([&of]
		{
		  if (of.is_open())
		  { of.close(); }
		});

		of.write(buf.get(), sizeof(ramdisk_header) + sizeof(ramdisk_item) * paths.size());
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
		exit(EX_IOERR);
	}

	cout << "Written metadata. Checksum is " << hex << header->checksum << endl;

	auto ret = copy_items(target, paths);
	if (ret != 0)
	{
		return ret;
	}

	auto align_bytes = header->size - size_total;

	try
	{
		ofstream of{ target, ios::binary | ios::app };
		auto _ = gsl::finally([&of]
		{
		  if (of.is_open())
		  { of.close(); }
		});

		auto align_buf = make_unique<char[]>(align_bytes);
		memset(align_buf.get(), 0, align_bytes);

		of.write(align_buf.get(), align_bytes);
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
		exit(EX_IOERR);
	}

	cout << "Written " << align_bytes << " byte(s) for alignment." << endl;

	return 0;
}
