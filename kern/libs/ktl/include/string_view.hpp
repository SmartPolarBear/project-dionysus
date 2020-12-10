#pragma once
// Imported from zircon kernel
// Previous license:
//
// Copyright 2020 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT


#include <string_view>

namespace ktl
{

	using std::basic_string_view;
	using std::string_view;

}  // namespace ktl

// Just including <ktl/string_view.h> enables "foo"sv literals.
using namespace std::literals::string_view_literals;

