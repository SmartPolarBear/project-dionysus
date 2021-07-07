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

#include "config.hpp"

#include "dependency.hpp"

#include <filesystem>
#include <span>
#include <tuple>
#include <queue>

using namespace std;
using namespace std::filesystem;

using namespace mkramdisk;
using namespace mkramdisk::configuration;

std::optional<std::vector<path>> mkramdisk::sort_by_dependency(const vector<item>& items)
{
	sort(items.begin(), items.end(), [](const item& a, const item& b)
	{
	  return a.id() < b.id();
	});

	vector<vector<int>> g(items.size(), vector<int>{});

	vector<int64_t> in_deg(items.size(), 0);
	for (const auto& item:items)
	{
		for (const auto& dep:item.deps())
		{
			in_deg[dep]++;
			g[item.id()].push_back(dep);
		}
	}

	queue<item> q;
	for (int i = 0; i < items.size(); i++)
	{
		if (!in_deg[i])
		{
			q.push(items[i]);
		}
	}

	vector<path> ret{};

	while (!q.empty())
	{
		auto n = q.front();
		q.pop();

		ret.emplace_back(n.path());

		auto e_remove = g[n.id()].back();
		in_deg[e_remove]--;
		g[n.id()].pop_back();

		if (!in_deg[e_remove])
		{
			q.push(items[e_remove]);
		}
	}

	for (const auto& n:g)
	{
		if (!n.empty())return std::nullopt;
	}

	return ret;
}
