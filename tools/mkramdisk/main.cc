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

#include <cstring>

#include <sysexits.h>

using namespace std;

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

	return 0;
}
