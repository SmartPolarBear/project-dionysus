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


#include "copy.hpp"
#include "round.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>

#include <gsl/gsl>

#include <sysexits.h>

using namespace std;
using namespace std::filesystem;

using namespace gsl;

int mkramdisk::copy_items(const std::filesystem::path& target, const std::vector<std::filesystem::path>& paths)
{
	ofstream of{ target, ios::binary | ios::app };
	auto _ = gsl::finally([&of]
	{
	  if (of.is_open())
	  { of.close(); }
	});

	for (const auto& i:paths)
	{
		cout << "Copy " << i;

		auto fsize = file_size(i);
		auto fbuf = make_unique<uint8_t[]>(roundup(fsize, sizeof(uint64_t)));
		memset(fbuf.get(), 0, roundup(fsize, sizeof(uint64_t)));

		ifstream ifs{ i, ios::binary };

		auto _2 = gsl::finally([&ifs]
		{
		  if (ifs.is_open())
		  {
			  ifs.close();
		  }
		});

		try
		{
			ifs.read(reinterpret_cast<char*>(fbuf.get()), fsize);
			if (!ifs)
			{
				cout << endl << "Error reading " << i << endl;
				return EX_IOERR;
			}
		}
		catch (const std::exception& e)
		{
			cout << e.what() << endl;
			return EX_IOERR;
		}

		try
		{
			of.write(reinterpret_cast<char*>(fbuf.get()), roundup(fsize, sizeof(uint64_t)));
			cout << " ," << "add " << hex << roundup(fsize, sizeof(uint64_t)) - fsize << " bytes for alignment."
			     << endl;

			if (!of)
			{
				cout << endl << "Error reading " << i << endl;
				return EX_IOERR;
			}
		}
		catch (const std::exception& e)
		{
			cout << e.what() << endl;
			return EX_IOERR;
		}

	}
	return 0;
}
