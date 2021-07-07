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
// Created by bear on 7/5/21.
//

#include <config.hpp>

using namespace mkramdisk::configuration;

const mkramdisk::configuration::item& mkramdisk::configuration::item::operator=(const mkramdisk::configuration::item& another) const
{
	id_ = another.id_;
	path_ = another.path_;

	deps_.clear();
	for (const auto& dep:another.deps_)
	{
		deps_.push_back(dep);
	}

	return *this;
}

void mkramdisk::configuration::to_json(nlohmann::json& j, const item& p)
{
	j = nlohmann::json{{ "id", p.id_ }, { "path", p.path_ }, { "deps_", p.deps_ }};
}

void mkramdisk::configuration::from_json(const nlohmann::json& j, item& p)
{
	j.at("id").get_to(p.id_);
	j.at("path").get_to(p.path_);
	j.at("deps").get_to(p.deps_);
}
