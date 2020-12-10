#pragma once
// Imported from zircon kernel
// Previous license:
//
// Copyright 2020 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <span>

namespace ktl {

using std::span;

using std::as_bytes;
using std::as_writable_bytes;

using std::dynamic_extent;

}  // namespace ktl
