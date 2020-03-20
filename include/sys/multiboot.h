#pragma once

#include "boot/multiboot2.h"
#include "sys/types.h"

// the *PHYSICAL* address for multiboot2_boot_info and the magic number
extern "C" void *mbi_structptr;
extern "C" uint32_t mbi_magicnum;

namespace multiboot
{

using multiboot_tag_ptr = multiboot_tag *;
using multiboot_tag_const_readonly_ptr = const multiboot_tag_ptr;

template <typename T>
using typed_mboottag_ptr = T *;

template <typename T>
using typed_mboottag_const_ptr = const typed_mboottag_ptr<T>;

template <typename T>
using typed_mboottag_const_readonly_ptr = typed_mboottag_const_ptr<T> const;

using aquire_tag_ptr_predicate = bool (*)(multiboot_tag_const_readonly_ptr);

constexpr size_t TAGS_COUNT_MAX = 24;

//This can be increase if the required info become more.
constexpr size_t BOOT_INFO_MAX_EXPECTED_SIZE = PAGE_SIZE;

void init_mbi(void);

// if buf==nullptr, regardless of bufsz, this function will return the number of tags with given type
// or it will copy specified amount of tags to the buf and return the count copied
size_t get_all_tags(size_t type, multiboot_tag_ptr *buf, size_t bufsz);

// get the first tag with the type
multiboot_tag_const_readonly_ptr acquire_tag_ptr(size_t type);
// get the first tag with the type, for which the predicate returns true
multiboot_tag_const_readonly_ptr acquire_tag_ptr(size_t type, aquire_tag_ptr_predicate pred);

template <typename T>
// get the first typed tag with the type
static inline auto acquire_tag_ptr(size_t type) -> typed_mboottag_const_readonly_ptr<T>
{
    return reinterpret_cast<typed_mboottag_const_readonly_ptr<T>>(acquire_tag_ptr(type));
}

template <typename T>
// get the first typed tag with the type, for which the predicate returns true
static inline auto acquire_tag_ptr(size_t type, aquire_tag_ptr_predicate pred) -> typed_mboottag_const_readonly_ptr<T>
{
    return reinterpret_cast<typed_mboottag_const_readonly_ptr<T>>(acquire_tag_ptr(type, pred));
}

} // namespace multiboot

