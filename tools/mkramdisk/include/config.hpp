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

#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <utility>

namespace mkramdisk::configuration
{
class item final
{
 public:

	item() = default;
	~item() = default;

	item(const item& another) : id_(another.id_), path_(another.path_)
	{
		deps_.clear();
		for (const auto& dep:another.deps_)
		{
			deps_.push_back(dep);
		}
	}

	const item& operator=(const item& another) const
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

	item(item&& another) noexcept: id_(std::exchange(another.id_, 0)),
	                               path_(std::exchange(another.path_, ""))
	{
		deps_.clear();
		for (const auto& dep:another.deps_)
		{
			deps_.push_back(dep);
		}
		another.deps_.clear();
	}

	[[nodiscard]] int32_t id() const
	{
		return id_;
	}

	[[nodiscard]] std::string path() const
	{
		return path_;
	}

	[[nodiscard]] std::vector<int32_t> deps() const
	{
		return deps_;
	}

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(item, id_, path_, deps_);

 private:
	mutable int32_t id_{ 0 };
	mutable std::string path_{};
	mutable std::vector<int32_t> deps_{};
};
}