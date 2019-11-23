#if !defined(__INCLUDE_SYS_MULTIBOOT_H)
#define __INCLUDE_SYS_MULTIBOOT_H

#include "boot/multiboot2.h"
#include "sys/types.h"

namespace multiboot
{

using multiboot_tag_ptr = multiboot_tag *;
using multiboot_tag_const_readonly_ptr = const multiboot_tag_ptr const;

template <typename T>
using typed_mboottag_ptr = T *;

template <typename T>
using typed_mboottag_const_ptr = const typed_mboottag_ptr<T>;

template <typename T>
using typed_mboottag_const_readonly_ptr = typed_mboottag_const_ptr<T> const;

constexpr size_t TAGS_COUNT_MAX = 24;

//This can be increase if the required info become more.
constexpr size_t BOOT_INFO_MAX_EXPECTED_SIZE = 4_KB;

void init_mbi(void);
void parse_multiboot_tags(void);

multiboot_tag_const_readonly_ptr aquire_tag_ptr(size_t type);

template <typename T>
static inline auto aquire_tag_ptr(size_t type) -> typed_mboottag_const_readonly_ptr<T>
{
    return reinterpret_cast<typed_mboottag_const_readonly_ptr<T>>(aquire_tag_ptr(type));
}

} // namespace multiboot

#endif // __INCLUDE_SYS_MULTIBOOT_H
