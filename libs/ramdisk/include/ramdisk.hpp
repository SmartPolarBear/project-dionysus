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
// Created by bear on 6/29/21.
//

#pragma once

#ifndef __cplusplus
#error "This file is only for C++"
#endif

#include <cstdint>

enum [[clang::enum_extensibility(closed)]] ramdisk_architecture : uint64_t
{
	ARCH_AMD64
};

static inline constexpr uint64_t RAMDISK_HEADER_MAGIC = 0x20011204;

struct ramdisk_item
{
	char name[16];
	uint64_t offset;
}__attribute__((packed));

static_assert(sizeof(ramdisk_item) % sizeof(uint64_t) == 0);


struct ramdisk_header
{
	uint64_t magic;
	ramdisk_architecture architecture;
	char name[16];
	uint64_t checksum;
	uint64_t size;
	uint64_t count;
	ramdisk_item items[0];
}__attribute__((packed));

static_assert(sizeof(ramdisk_header) % sizeof(uint64_t) == 0);
